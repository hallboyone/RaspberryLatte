#include "pid.h"
#include <string.h>

void discrete_derivative_init(discrete_derivative * d, uint filter_span_ms){
    d->filter_span_ms = filter_span_ms;
    d->_buf_len = 10;
    d->_num_el = 0;
    d->_data = malloc((d->_buf_len)*sizeof(datapoint));
}

float discrete_derivative_read(discrete_derivative * d){
    if(d->_num_el < 2){
        return 0;
    }
    float v_sum = 0;
    uint64_t t_sum = 0;
    for(uint16_t i = 0; i < d->_num_el; i++){
        v_sum += d->_data[i].v;
        t_sum += d->_data[i].t;
    }

    const float v_avg = v_sum/(float)d->_num_el;
    const float t_avg = t_sum/(float)d->_num_el;
    
    float num = 0;
    float dem = 0;
    for(uint16_t i = 0; i < d->_num_el; i++){
        num = num + (d->_data[i].t-t_avg)*(d->_data[i].v-v_avg);
        dem = dem + ((d->_data[i].t-t_avg)*(d->_data[i].t-t_avg));
    }
    return num/dem;
}

/**
 * \brief Removes all points in d's _data buf with timestamps more than d->filter_span_ms
 * millaseconds behind cur_t. If all points are outside of this range, the most recent point is 
 * always kept.
 */
void _discrete_derivative_remove_old_points(discrete_derivative * d, uint64_t cur_t){
    uint16_t keep_from_idx = 0;
    for(keep_from_idx; keep_from_idx < d->_num_el-1; keep_from_idx++){
        // Find first point within the time range from cur_t
        if(cur_t - d->_data[keep_from_idx].t <= d->filter_span_ms){
            break;
        }
    }
    // If this is not the first point in array
    if(keep_from_idx > 0){
        // Shift first point to start of array
        d->_num_el -= keep_from_idx;
        memcpy(d->_data, &(d->_data[keep_from_idx]), (d->_num_el)*sizeof(datapoint));
    }

}

/**
 * \brief Doubles the size of the _data buffer in d.
 */
void _discrete_derivative_expand_buf(discrete_derivative * d){
    d->_buf_len *= 2;
    datapoint * new_buf = malloc(sizeof(datapoint) * d->_buf_len);
    memcpy(new_buf, d->_data, (d->_num_el)*sizeof(datapoint));
    free(d->_data);
    d->_data = new_buf;
}

float discrete_derivative_add_point(discrete_derivative * d, datapoint p){
    _discrete_derivative_remove_old_points(d, p.t);
    if(d->_num_el == d->_buf_len){
        _discrete_derivative_expand_buf(d);
    }
    d->_data[d->_num_el] = p;
    d->_num_el += 1;
    return discrete_derivative_read(d);
}