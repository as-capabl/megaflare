#pragma once

namespace std {
    template <typename Iterator>
    inline Iterator begin(std::pair<Iterator, Iterator>& pair)
    {
        return pair.first;
    }

    template <typename Iterator>
    inline Iterator end(std::pair<Iterator, Iterator>& pair)
    {
        return pair.second;
    }
}

