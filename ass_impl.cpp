#include "ass.hpp"
#include <numeric>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#endif

namespace myelin {

ASS::ASS(int total_channels, int cluster_size) 
    : m_total_channels(total_channels), m_cluster_size(cluster_size) {
}

void ASS::apply(uint16_t* frame_ptr, int samples) {
    int num_clusters = m_total_channels / m_cluster_size;
    m_cluster_averages.resize(samples * num_clusters, 0);

    uint16_t* __restrict avg_ptr = m_cluster_averages.data();

    for (int s = 0; s < samples; ++s) {
        int sample_offset = s * m_total_channels;
        int avg_offset = s * num_clusters;
        
        for (int cluster = 0; cluster < num_clusters; ++cluster) {
            int base_idx = sample_offset + cluster * m_cluster_size;
            uint32_t sum = 0;
            
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
            // Optimize SUM with NEON if cluster size is multiple of 8
            if (m_cluster_size % 8 == 0) {
                uint32x4_t sum_vec = vdupq_n_u32(0);
                for (int c = 0; c < m_cluster_size; c += 8) {
                    uint16x8_t vals = vld1q_u16(&frame_ptr[base_idx + c]);
                    // vaddl_u16 expands to 32-bit halves
                    sum_vec = vaddq_u32(sum_vec, vaddl_u16(vget_low_u16(vals), vget_high_u16(vals)));
                }
                sum = vgetq_lane_u32(sum_vec, 0) + vgetq_lane_u32(sum_vec, 1) + 
                      vgetq_lane_u32(sum_vec, 2) + vgetq_lane_u32(sum_vec, 3);
            } else {
#endif
                for (int c = 0; c < m_cluster_size; ++c) {
                    sum += frame_ptr[base_idx + c];
                }
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
            }
#endif
            
            uint16_t avg = static_cast<uint16_t>(sum / m_cluster_size);
            avg_ptr[avg_offset + cluster] = avg;
            
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
            if (m_cluster_size % 8 == 0) {
                int32x4_t v_avg_32 = vdupq_n_s32(avg);
                int32x4_t v_512_32 = vdupq_n_s32(512);
                int32x4_t v_1023 = vdupq_n_s32(1023);
                int32x4_t v_0 = vdupq_n_s32(0);

                for (int c = 0; c < m_cluster_size; c += 8) {
                    uint16x8_t vals = vld1q_u16(&frame_ptr[base_idx + c]);
                    
                    // Low half
                    int32x4_t low = vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(vals)));
                    low = vaddq_s32(vsubq_s32(low, v_avg_32), v_512_32);
                    low = vminq_s32(vmaxq_s32(low, v_0), v_1023);
                    
                    // High half
                    int32x4_t high = vreinterpretq_s32_u32(vmovl_u16(vget_high_u16(vals)));
                    high = vaddq_s32(vsubq_s32(high, v_avg_32), v_512_32);
                    high = vminq_s32(vmaxq_s32(high, v_0), v_1023);

                    // Combine and store
                    uint16x8_t res = vcombine_u16(vqmovun_s32(low), vqmovun_s32(high));
                    vst1q_u16(&frame_ptr[base_idx + c], res);
                }
            } else {
#endif
                for (int c = 0; c < m_cluster_size; ++c) {
                    int32_t val = static_cast<int32_t>(frame_ptr[base_idx + c]) - avg + 512;
                    if (val < 0) val = 0;
                    else if (val > 1023) val = 1023;
                    frame_ptr[base_idx + c] = static_cast<uint16_t>(val);
                }
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
            }
#endif
        }
    }
}

void ASS::reconstruct(uint16_t* frame_ptr, int samples) {
    int num_clusters = m_total_channels / m_cluster_size;
    uint16_t* __restrict avg_ptr = m_cluster_averages.data();
    
    for (int s = 0; s < samples; ++s) {
        int sample_offset = s * m_total_channels;
        int avg_offset = s * num_clusters;
        
        for (int cluster = 0; cluster < num_clusters; ++cluster) {
            uint16_t avg = avg_ptr[avg_offset + cluster];
            int base_idx = sample_offset + cluster * m_cluster_size;
            
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
            if (m_cluster_size % 8 == 0) {
                int32x4_t v_avg_32 = vdupq_n_s32(avg);
                int32x4_t v_512_32 = vdupq_n_s32(512);
                int32x4_t v_1023 = vdupq_n_s32(1023);
                int32x4_t v_0 = vdupq_n_s32(0);

                for (int c = 0; c < m_cluster_size; c += 8) {
                    uint16x8_t vals = vld1q_u16(&frame_ptr[base_idx + c]);
                    
                    // Low half
                    int32x4_t low = vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(vals)));
                    low = vsubq_s32(vaddq_s32(low, v_avg_32), v_512_32);
                    low = vminq_s32(vmaxq_s32(low, v_0), v_1023);
                    
                    // High half
                    int32x4_t high = vreinterpretq_s32_u32(vmovl_u16(vget_high_u16(vals)));
                    high = vsubq_s32(vaddq_s32(high, v_avg_32), v_512_32);
                    high = vminq_s32(vmaxq_s32(high, v_0), v_1023);

                    // Combine and store
                    uint16x8_t res = vcombine_u16(vqmovun_s32(low), vqmovun_s32(high));
                    vst1q_u16(&frame_ptr[base_idx + c], res);
                }
            } else {
#endif
                for (int c = 0; c < m_cluster_size; ++c) {
                    int32_t val = static_cast<int32_t>(frame_ptr[base_idx + c]) + avg - 512;
                    if (val < 0) val = 0;
                    else if (val > 1023) val = 1023;
                    frame_ptr[base_idx + c] = static_cast<uint16_t>(val);
                }
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
            }
#endif
        }
    }
}

} // namespace myelin
