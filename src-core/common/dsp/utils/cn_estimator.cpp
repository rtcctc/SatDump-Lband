#include "cn_estimator.h"
#include <cmath>
#include <algorithm>
#include <complex>

namespace dsp
{
    CNEstimator::CNEstimator(int fft_size) : d_fft_size(fft_size)
    {
        d_buffer.resize(d_fft_size);
    }

    void CNEstimator::fft_radix2(std::complex<float> *data, int n)
    {
        for (int i = 1, j = 0; i < n; i++)
        {
            int bit = n >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) std::swap(data[i], data[j]);
        }
        for (int len = 2; len <= n; len <<= 1)
        {
            float ang = -2.0f * (float)M_PI / len;
            std::complex<float> wlen(cosf(ang), sinf(ang));
            for (int i = 0; i < n; i += len)
            {
                std::complex<float> w(1.0f, 0.0f);
                for (int j = 0; j < len / 2; j++)
                {
                    std::complex<float> u = data[i + j];
                    std::complex<float> v = data[i + j + len / 2] * w;
                    data[i + j] = u + v;
                    data[i + j + len / 2] = u - v;
                    w *= wlen;
                }
            }
        }
    }

    void CNEstimator::update(complex_t *input, int size)
    {
        for (int i = 0; i < size; i++)
        {
            d_buffer[d_buffer_idx++] = input[i];
            if (d_buffer_idx >= d_fft_size)
            {
                float cn = compute_cn();
                if (cn == cn) // not NaN
                {
                    d_history.push_back(cn);
                    if ((int)d_history.size() > HISTORY_SIZE)
                        d_history.pop_front();
                }
                d_buffer_idx = 0;
            }
        }
    }

    float CNEstimator::compute_cn()
    {
        // Копируем в std::complex<float> и применяем окно Ханна
        std::vector<std::complex<float>> work(d_fft_size);
        for (int i = 0; i < d_fft_size; i++)
        {
            float w = 0.5f * (1.0f - cosf(2.0f * (float)M_PI * i / (d_fft_size - 1)));
            std::complex<float> s = (std::complex<float>)d_buffer[i];
            work[i] = s * w;
        }

        fft_radix2(work.data(), d_fft_size);

        // Мощность спектра
        std::vector<float> power(d_fft_size / 2);
        for (int i = 0; i < d_fft_size / 2; i++)
            power[i] = std::norm(work[i]);

        // Пик = несущая
        int peak_bin = (int)(std::max_element(power.begin(), power.end()) - power.begin());
        float peak_power = power[peak_bin];

        // Шумовой пол = медиана бинов, исключая окрестность пика
        std::vector<float> noise_bins;
        noise_bins.reserve(power.size());
        int exclude = std::max(3, d_fft_size / 128);
        for (int i = 0; i < (int)power.size(); i++)
            if (std::abs(i - peak_bin) > exclude)
                noise_bins.push_back(power[i]);

        if (noise_bins.empty())
            return 0.0f;

        std::nth_element(noise_bins.begin(),
                         noise_bins.begin() + noise_bins.size() / 2,
                         noise_bins.end());
        float noise_floor = noise_bins[noise_bins.size() / 2];

        float cn_linear = (peak_power - noise_floor) / std::max(noise_floor, 1e-20f);
        return 10.0f * log10f(std::max(cn_linear, 1e-10f));
    }

    float CNEstimator::cn() const
    {
        if (d_history.empty()) return 0.0f;
        float sum = 0;
        for (float v : d_history) sum += v;
        return sum / d_history.size();
    }
}