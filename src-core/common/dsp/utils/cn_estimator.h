#pragma once
#include "common/dsp/complex.h"
#include <vector>
#include <deque>
#include <complex>

namespace dsp
{
    class CNEstimator
    {
    private:
        int d_fft_size;
        std::vector<complex_t> d_buffer;
        int d_buffer_idx = 0;

        std::deque<float> d_history;
        static constexpr int HISTORY_SIZE = 32;

        // FFT на std::complex<float> — у SatDump's complex_t нет нужных операторов
        static void fft_radix2(std::complex<float> *data, int n);

        float compute_cn();

    public:
        CNEstimator(int fft_size = 1024);
        void update(complex_t *input, int size);
        float cn() const;
        bool has_value() const { return !d_history.empty(); }
    };
}