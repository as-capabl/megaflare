#pragma once

#include <CL/cl.h>
#include <atomic>
#include <boost/intrusive_ptr.hpp>

void intrusive_ptr_add_ref(_cl_event* i_ptr)
{
    ::clRetainEvent(i_ptr);
}

void intrusive_ptr_release(_cl_event* i_ptr)
{
    ::clReleaseEvent(i_ptr);
}

namespace megaflare {
    namespace host {
        struct event  {
            typedef boost::intrusive_ptr<_cl_event> checked_out_t;

            event() : m_atomPtr(nullptr) {}

            //::clRetainEventが必要なら自分で。
            event(cl_event i_pEv) : m_atomPtr(i_pEv) {}

            event(event const & i_ev) : m_atomPtr(nullptr){
                if (cl_event pEv = i_ev.m_atomPtr.load() ) {
                    if( ::clRetainEvent(pEv) == CL_SUCCESS ) {
                        m_atomPtr.store(pEv);
                    }
                }
            }

            cl_event operator= (cl_event const & i_ev) {
                m_atomPtr.store(i_ev);
                return i_ev;
            }


            ~event() {
                if( cl_event pEv = m_atomPtr.load() ) {
                    ::clReleaseEvent(pEv);
                }
            }

            checked_out_t exchange(checked_out_t && i_ptr) {
                ::clRetainEvent(i_ptr.get());

                cl_event pOld = m_atomPtr.load(std::memory_order_acq_rel);
                while( pOld == busy_state()
                       || !m_atomPtr.compare_exchange_strong(
                           pOld, 
                           i_ptr.get(),
                           std::memory_order_acq_rel
                       ))
                {
                    std::this_thread::yield();
                    pOld = m_atomPtr.load(std::memory_order_acq_rel);
                }
                return pOld;
            }

            checked_out_t lock_and_checkout() {
                cl_event pOld = m_atomPtr.load(std::memory_order_acquire);
                while( pOld == busy_state()
                       || !m_atomPtr.compare_exchange_strong(
                           pOld, 
                           busy_state(),
                           std::memory_order_acquire
                       ))
                {
                    std::this_thread::yield();
                    pOld = m_atomPtr.load(std::memory_order_acquire);
                }
                return pOld;
            }

            void checkin_and_unlock(checked_out_t && i_coNew)
            {
                cl_event pNew = i_coNew.get();
                if( pNew != nullptr  ) {
                    ::clRetainEvent(pNew);
                }
                cl_event pOld = m_atomPtr.exchange(
                    pNew, 
                    std::memory_order_release );
                if( pOld != nullptr && pOld != busy_state() ) {
                    ::clReleaseEvent(pOld);
                }
            }
            
            cl_event get() const {
                return m_atomPtr.load();
            }

            //単なるユーティリティ
            //別に生のものをgetしてclWaitForEventsしてしまっても良い。
            void wait() {
                cl_event pEv = m_atomPtr.load();
                clWaitForEvents(1, &pEv);
            }

        private:
            std::atomic<cl_event> volatile m_atomPtr;

            //自分と同じポインタ値になる事だけは絶対にないだろうという目論見のもと
            //nullptr以外のフラグ値として使う。
            cl_event busy_state() {
                return reinterpret_cast<cl_event>(this);
            }
        };
    }
}
