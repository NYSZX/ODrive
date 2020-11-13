#ifndef __CONTROLLER_HPP
#define __CONTROLLER_HPP

#include <array>

class Controller : public ODriveIntf::ControllerIntf {
public:
    struct Anticogging_t{
        std::array<float, 1024> cogging_map = {0}; // [Nm]
        float max_torque = 0.15f; // [Nm]
        bool pre_calibrated = false;
        bool calib_anticogging = false;
        bool anticogging_enabled = true;
        float start_vel = 0.5f; // [turns/s]
        float end_vel = 0.05f; // [turns/s]
        float start_gain = 25.0f;
        float end_gain = 5.0f;
        float end_tolerance = 0.1f; // threshold for average abs vel error to end anticogging
    };

    struct Config_t {
        ControlMode control_mode = CONTROL_MODE_POSITION_CONTROL;  //see: ControlMode_t
        InputMode input_mode = INPUT_MODE_PASSTHROUGH;             //see: InputMode_t
        float pos_gain = 20.0f;                  // [(turn/s) / turn]
        float vel_gain = 1.0f / 6.0f;            // [Nm/(turn/s)]
        // float vel_gain = 0.2f / 200.0f,       // [Nm/(rad/s)] <sensorless example>
        float vel_integrator_gain = 2.0f / 6.0f; // [Nm/(turn/s * s)]
        float vel_limit = 2.0f;                  // [turn/s] Infinity to disable.
        float vel_limit_tolerance = 1.2f;        // ratio to vel_lim. Infinity to disable.
        float vel_ramp_rate = 1.0f;              // [(turn/s) / s]
        float torque_ramp_rate = 0.01f;          // Nm / sec
        bool circular_setpoints = false;
        float circular_setpoint_range = 1.0f; // Circular range when circular_setpoints is true. [turn]
        float inertia = 0.0f;                 // [Nm/(turn/s^2)]
        float input_filter_bandwidth = 2.0f;  // [1/s]
        float homing_speed = 0.25f;           // [turn/s]
        Anticogging_t anticogging;
        float gain_scheduling_width = 10.0f;
        bool enable_gain_scheduling = false;
        bool enable_vel_limit = true;
        bool enable_overspeed_error = true;
        bool enable_current_mode_vel_limit = true;  // enable velocity limit in current control mode (requires a valid velocity estimator)
        uint8_t axis_to_mirror = -1;
        float mirror_ratio = 1.0f;
        uint8_t load_encoder_axis = -1;  // default depends on Axis number and is set in load_configuration(). Set to -1 to select sensorless estimator.

        // custom setters
        Controller* parent;
        void set_input_filter_bandwidth(float value) { input_filter_bandwidth = value; parent->update_filter_gains(); }
    };

    Controller() {}
    
    bool apply_config();

    void reset();
    void set_error(Error error);

    constexpr void input_pos_updated() {
        input_pos_updated_ = true;
    }

    bool select_encoder(size_t encoder_num);

    // Trajectory-Planned control
    void move_to_pos(float goal_point);
    void move_incremental(float displacement, bool from_goal_point);
    
    // TODO: make this more similar to other calibration loops
    void start_anticogging_calibration();
    void stop_anticogging_calibration();
    void anticogging_calibration(float pos_estimate, float vel_estimate, float vel_setpoint);
    void anticogging_remove_bias();
    float anticogging_get_val(uint32_t index);
    void anticogging_set_val(uint32_t index, float val);

    void update_filter_gains();
    bool update();

    Config_t config_;
    Axis* axis_ = nullptr; // set by Axis constructor

    Error error_ = ERROR_NONE;

    // Inputs
    InputPort<float> pos_estimate_linear_src_;
    InputPort<float> pos_estimate_circular_src_;
    InputPort<float> vel_estimate_src_;
    InputPort<float> pos_wrap_src_; 

    float pos_setpoint_ = 0.0f; // [turns]
    float vel_setpoint_ = 0.0f; // [turn/s]
    // float vel_setpoint = 800.0f; <sensorless example>
    float vel_integrator_torque_ = 0.0f;    // [Nm]
    float torque_setpoint_ = 0.0f;  // [Nm]

    float input_pos_ = 0.0f;     // [turns]
    float input_vel_ = 0.0f;     // [turn/s]
    float input_torque_ = 0.0f;  // [Nm]
    float input_filter_kp_ = 0.0f;
    float input_filter_ki_ = 0.0f;

    bool input_pos_updated_ = false;
    
    bool trajectory_done_ = true;

    bool anticogging_valid_ = false;

    float anticogging_error_max_ = -std::numeric_limits<float>::infinity();
    float anticogging_average_error_ = 0.0f;
    float anticogging_start_pos_ = 0.0f;
    float old_vel_integrator_gain_ = 0.0f;
    int32_t anticogging_turn_count_ = 0;
    float anticogging_vel_error_filtered_ = 0.0f;
    float anticogging_integrator_gain_ = 0.0f; // [Nm/s / (turns/s)]
    float anticogging_bandwidth_ = 0.4f; // [rad/s]
    // Outputs
    OutputPort<float> torque_output_ = 0.0f;

    // custom setters
    void set_input_pos(float value) { input_pos_ = value; input_pos_updated(); }
};

#endif // __CONTROLLER_HPP
