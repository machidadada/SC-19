#define DISTANCE_AVERAGE_BUF_SIZE   (3)
static int16_t distance_average_buf[DISTANCE_AVERAGE_BUF_SIZE];

extern int16_t distance_sensor_value;

void calc_distance_average(void) {
    int index;
    int32_t distance_sum = 0;

    //Step forward buffer value and set latest valuet to buffer.
    for (index = 0; index < DISTANCE_AVERAGE_BUF_SIZE - 1; index++) {
        distance_average_buf[index] = distance_average_buf[index + 1];
    }
    distance_average_buf[DISTANCE_AVERAGE_BUF_SIZE - 1] =
        distance_sensor_value;

    for (index = 0; index < DISTANCE_AVERAGE_BUF_SIZE; index++) {
        distance_sum += (int32_t)distance_average_buf[index];
    }

    distance_average_value =
        (int16_t)(distance_sum / DISTANCE_AVERAGE_BUF_SIZE);
}

const int8_t CAR_SAFE_STATE_SAFE = 0;
const int8_t CAR_SAFE_STATE_ATTN = 1;
const int8_t CAR_SAFE_STATE_DANG = 2;
const int8_t CAR_SAFE_STATE_STOP = 3;

static const int16_t CAR_SAFE_STATE_SAFE_ATTN_BORDER = 200;  //安全→警告
static const int16_t CAR_SAFE_STATE_ATTN_SAFE_BORDER = 210;  //警告→安全
static const int16_t CAR_SAFE_STATE_ATTN_DANG_BORDER = 140;  //注意→危険
static const int16_t CAR_SAFE_STATE_DANG_ATTN_BORDER = 150;  //危険→注意
static const int16_t CAR_SAFE_STATE_STOP_DANG_BORDER = 50;   //停止→危険
static const int16_t CAR_SAFE_STATE_DANG_STOP_BORDER = 60;   //危険→停止

void judge_dist_safe(void) {
    int16_t distance_average_value_tmp;
    
    //Latch latest value.
    distance_average_value_tmp = distance_average_value;

    if (CAR_SAFE_STATE_SAFE == distance_safe_state) {
        if (CAR_SAFE_STATE_SAFE_ATTN_BORDER > distance_average_value_tmp) {
            distance_safe_state = CAR_SAFE_STATE_ATTN;
        }
    } else if (CAR_SAFE_STATE_ATTN == distance_safe_state) {
        if (CAR_SAFE_STATE_ATTN_DANG_BORDER > distance_average_value_tmp) {
            distance_safe_state = CAR_SAFE_STATE_DANG;
        }
        if (CAR_SAFE_STATE_ATTN_SAFE_BORDER < distance_average_value_tmp) {
            distance_safe_state = CAR_SAFE_STATE_SAFE;
        }
    } else if (CAR_SAFE_STATE_DANG == distance_safe_state) {
        if (CAR_SAFE_STATE_DANG_STOP_BORDER > distance_average_value_tmp) {
            distance_safe_state = CAR_SAFE_STATE_STOP;
        }
        if (CAR_SAFE_STATE_DANG_ATTN_BORDER < distance_average_value_tmp) {
            distance_safe_state = CAR_SAFE_STATE_ATTN;
        }
    } else if (CAR_SAFE_STATE_STOP == distance_safe_state) {
        if (CAR_SAFE_STATE_STOP_DANG_BORDER < distance_average_value_tmp) {
            distance_safe_state = CAR_SAFE_STATE_DANG;
        }
    }
}