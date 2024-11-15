#pragma once

#include "Vector.hpp"
#include <cmath>

class LshHash {
    private:
        Vector<float> v;
        float t;
    public:
        LshHash(uint32_t size, uint32_t window) 
        : v(size, NORMAL, 0, 1. / window), t(Vector<float>(1, UNIFORM, 0, 1.)[0]) { }
        ~LshHash() {}
        
        template <typename T>
        uint32_t apply(Vector<T>& p) { return std::floor(v * p + t); }
};



class LshAmplifiedHash {
    private:
        uint32_t k;
        Vector<uint32_t> r;
        std::vector<LshHash*> h;
    public:
        LshAmplifiedHash(uint32_t size, uint32_t window, uint32_t k_)
        : k(k_), r(k, UNIFORM, 0, UINT32_MAX) {
            for (uint32_t i = 0; i < k; i++)
                h.push_back(new LshHash(size, window));
        }
        
        ~LshAmplifiedHash() {            
            for (auto hash : h)
                delete hash;
        }

        template <typename T>
        uint32_t apply(Vector<T>& p) { 
            uint32_t M = UINT32_MAX - 4; // 2^32 - 1 == UINT32_MAX --> UINT32_MAX - 4 = (UINT32_MAX + 1) - 5 = 2^32 - 5

            uint32_t sum = 0;
            
            // Linear combination
            for (uint32_t i = 0; i < k; i++)
                sum += r[i] * h[i]->apply(p) % M;

            return sum % M;
        }
};