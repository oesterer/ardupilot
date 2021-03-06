/// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-
#ifndef AC_WPNAV_H
#define AC_WPNAV_H

#include <inttypes.h>
#include <AP_Common.h>
#include <AP_Param.h>
#include <AP_Math.h>
#include <AC_PID.h>             // PID library
#include <APM_PI.h>             // PID library
#include <AP_InertialNav.h>     // Inertial Navigation library

// loiter maximum velocities and accelerations
#define MAX_LOITER_POS_VELOCITY         500         // maximum velocity that our position controller will request.  should be 1.5 ~ 2.0 times the pilot input's max velocity.  To-Do: make consistent with maximum velocity requested by pilot input to loiter
#define MAX_LOITER_POS_ACCEL            250         // defines the velocity vs distant curve.  maximum acceleration in cm/s/s that loiter position controller asks for from acceleration controller
#define MAX_LOITER_VEL_ACCEL            800         // max acceleration in cm/s/s that the loiter velocity controller will ask from the lower accel controller.
                                                    // should be 1.5 times larger than MAX_LOITER_POS_ACCEL.
                                                    // max acceleration = max lean angle * 980 * pi / 180.  i.e. 23deg * 980 * 3.141 / 180 = 393 cm/s/s

#define MAX_LEAN_ANGLE                  4500        // default maximum lean angle

#define MAX_LOITER_OVERSHOOT            531        // maximum distance (in cm) that we will allow the target loiter point to be from the current location when switching into loiter
// D0 = MAX_LOITER_POS_ACCEL/(2*Pid_P^2);
// if MAX_LOITER_POS_VELOCITY > 2*D0*Pid_P
//     MAX_LOITER_OVERSHOOT = D0 + MAX_LOITER_POS_VELOCITY.^2 ./ (2*MAX_LOITER_POS_ACCEL);
// else
//     MAX_LOITER_OVERSHOOT = min(200, MAX_LOITER_POS_VELOCITY/Pid_P); // to stop it being over sensitive to error
// end

#define WPNAV_WP_SPEED                  500         // default horizontal speed betwen waypoints in cm/s
#define WPNAV_WP_RADIUS                 200         // default waypoint radius in cm
#define WPINAV_MAX_POS_ERROR            531.25f     // maximum distance (in cm) that the desired track can stray from our current location.
// D0 = MAX_LOITER_POS_ACCEL/(2*Pid_P^2);
// if WP_SPEED > 2*D0*Pid_P
//     WPINAV_MAX_POS_ERROR = D0 + WP_SPEED.^2 ./ (2*MAX_LOITER_POS_ACCEL);
// else
//     WPINAV_MAX_POS_ERROR = min(200, WP_SPEED/Pid_P); // to stop it being over sensitive to error
// end
// This should use the current waypoint max speed though rather than the default

#define MAX_CLIMB_VELOCITY              125         // maximum climb velocity - ToDo: pull this in from main code
#define WPINAV_MAX_ALT_ERROR            100.0f      // maximum distance (in cm) that the desired track can stray from our current location.
// D0 = ALT_HOLD_ACCEL_MAX/(2*Pid_P^2);
// if g.pilot_velocity_z_max > 2*D0*Pid_P
//     WPINAV_MAX_ALT_ERROR = D0 + MAX_CLIMB_VELOCITY.^2 ./ (2*ALT_HOLD_ACCEL_MAX);
// else
//     WPINAV_MAX_ALT_ERROR = min(100, MAX_CLIMB_VELOCITY/Pid_P); // to stop it being over sensitive to error
// end
class AC_WPNav
{
public:

    /// Constructor
    AC_WPNav(AP_InertialNav* inav, APM_PI* pid_pos_lat, APM_PI* pid_pos_lon, AC_PID* pid_rate_lat, AC_PID* pid_rate_lon);

    ///
    /// simple loiter controller
    ///

    /// get_loiter_target - get loiter target as position vector (from home in cm)
    Vector3f get_loiter_target() { return _target; }

    /// set_loiter_target in cm from home
    void set_loiter_target(const Vector3f& position) { _target = position; }

    /// set_loiter_target - set initial loiter target based on current position and velocity
    void set_loiter_target(const Vector3f& position, const Vector3f& velocity);

    /// move_loiter_target - move destination using pilot input
    void move_loiter_target(float control_roll, float control_pitch, float dt);

    /// get_distance_to_target - get horizontal distance to loiter target in cm
    float get_distance_to_target();

    /// get_bearing_to_target - get bearing to loiter target in centi-degrees
    int32_t get_bearing_to_target();

    /// update_loiter - run the loiter controller - should be called at 10hz
    void update_loiter();

    /// set_angle_limit - limits maximum angle in centi-degrees the copter will lean
    void set_angle_limit(int32_t lean_angle) { _lean_angle_max = lean_angle; }

    /// clear_angle_limit - reset angle limits back to defaults
    void clear_angle_limit() { _lean_angle_max = MAX_LEAN_ANGLE; }

    /// get_angle_limit - retrieve maximum angle in centi-degrees the copter will lean
    int32_t get_angle_limit() { return _lean_angle_max; }

    ///
    /// waypoint controller
    ///

    /// get_destination waypoint using position vector (distance from home in cm)
    Vector3f get_destination() { return _destination; }

    /// set_destination waypoint using position vector (distance from home in cm)
    void set_destination(const Vector3f& destination);

    /// set_origin_and_destination - set origin and destination waypoints using position vectors (distance from home in cm)
    void set_origin_and_destination(const Vector3f& origin, const Vector3f& destination);

    /// advance_target_along_track - move target location along track from origin to destination
    void advance_target_along_track(float velocity_cms, float dt);

    /// get_distance_to_destination - get horizontal distance to destination in cm
    float get_distance_to_destination();

    /// get_bearing_to_destination - get bearing to next waypoint in centi-degrees
    int32_t get_bearing_to_destination();

    /// reached_destination - true when we have come within RADIUS cm of the waypoint
    bool reached_destination() { return _reached_destination; }

    /// update_wp - update waypoint controller
    void update_wpnav();

    ///
    /// shared methods
    ///

    /// get desired roll, pitch which should be fed into stabilize controllers
    int32_t get_desired_roll() { return _desired_roll; };
    int32_t get_desired_pitch() { return _desired_pitch; };

    /// get_desired_alt - get desired altitude (in cm above home) from loiter or wp controller which should be fed into throttle controller
    float get_desired_alt() { return _target.z; }

    /// set_desired_alt - set desired altitude (in cm above home)
    void set_desired_alt(float desired_alt) { _target.z = desired_alt; }

    /// set_cos_sin_yaw - short-cut to save on calculations to convert from roll-pitch frame to lat-lon frame
    void set_cos_sin_yaw(float cos_yaw, float sin_yaw, float cos_roll) {
        _cos_yaw = cos_yaw;
        _sin_yaw = sin_yaw;
        _cos_roll = cos_roll;
    }

    /// set_horizontal_velocity - allows main code to pass target horizontal velocity for wp navigation
    void set_horizontal_velocity(float velocity_cms) { _speed_cms = velocity_cms; };

    /// set_climb_velocity - allows main code to pass max climb velocity to wp navigation
    void set_climb_velocity(float velocity_cms) { _speedz_cms = velocity_cms; };

    /// get_waypoint_radius - access for waypoint radius in cm
    float get_waypoint_radius() { return _wp_radius_cm; }

    static const struct AP_Param::GroupInfo var_info[];

protected:

    /// project_stopping_point - returns vector to stopping point based on a horizontal position and velocity
    Vector3f project_stopping_point(const Vector3f& position, const Vector3f& velocity);

    /// translate_loiter_target_movements - consumes adjustments created by move_loiter_target
    void translate_loiter_target_movements(float nav_dt);

    /// get_loiter_position_to_velocity - loiter position controller
    ///     converts desired position held in _target vector to desired velocity
    void get_loiter_position_to_velocity(float dt);

    /// get_loiter_velocity_to_acceleration - loiter velocity controller
    ///    converts desired velocities in lat/lon directions to accelerations in lat/lon frame
    void get_loiter_velocity_to_acceleration(float vel_lat_cms, float vel_lon_cms, float dt);

    /// get_loiter_acceleration_to_lean_angles - loiter acceleration controller
    ///    converts desired accelerations provided in lat/lon frame to roll/pitch angles
    void get_loiter_acceleration_to_lean_angles(float accel_lat_cmss, float accel_lon_cmss);

    /// get_bearing_cd - return bearing in centi-degrees between two positions
    float get_bearing_cd(const Vector3f origin, const Vector3f destination);

    /// reset_I - clears I terms from loiter PID controller
    void reset_I();

    // pointers to inertial nav library
    AP_InertialNav*	_inav;

    // pointers to pid controllers
    APM_PI*		_pid_pos_lat;
    APM_PI*		_pid_pos_lon;
    AC_PID*		_pid_rate_lat;
    AC_PID*		_pid_rate_lon;

    // parameters
    AP_Float    _speed_cms;         // default horizontal speed in cm/s
    float       _speedz_cms;        // max vertical climb rate in cm/s.  To-Do: rename or pull this from main code
    AP_Float    _wp_radius_cm;      // distance from a waypoint in cm that, when crossed, indicates the wp has been reached
    uint32_t	_loiter_last_update;    // time of last update_loiter call
    uint32_t	_wpnav_last_update;     // time of last update_wpnav call
    float       _cos_yaw;           // short-cut to save on calcs required to convert roll-pitch frame to lat-lon frame
    float       _sin_yaw;
    float       _cos_roll;

    // output from controller
    int32_t     _desired_roll;          // fed to stabilize controllers at 50hz
    int32_t     _desired_pitch;         // fed to stabilize controllers at 50hz

    int32_t     _lean_angle_max;        // maximum lean angle.  can we set from main code so that throttle controller can stop leans that cause copter to lose altitude

    // internal variables
    Vector3f    _target;   		        // loiter's target location in cm from home
    Vector3f    _target_vel;            // loiter
    Vector3f    _vel_last;              // previous iterations velocity in cm/s
    Vector3f    _origin;                // starting point of trip to next waypoint in cm from home (equivalent to next_WP)
    Vector3f    _destination;           // target destination in cm from home (equivalent to next_WP)
    Vector3f    _pos_delta_unit;        // each axis's percentage of the total track from origin to destination
    float       _track_length;          // distance in cm between origin and destination
    float       _track_desired;         // our desired distance along the track in cm
    float       _distance_to_target;    // distance to loiter target
    float       _vert_track_scale;      // vertical scaling to give altitude equal weighting to position
    bool        _reached_destination;   // true if we have reached the destination

    // pilot inputs for loiter
    int16_t     _pilot_vel_forward_cms;
    int16_t     _pilot_vel_right_cms;

public:
    // for logging purposes
    Vector2f dist_error;                // distance error calculated by loiter controller
    Vector2f desired_vel;               // loiter controller desired velocity
    Vector2f desired_accel;             // the resulting desired acceleration
    
    // To-Do: add split of fast (100hz for accel->angle) and slow (10hz for navigation)
    /// update - run the loiter and wpnav controllers - should be called at 100hz
    //void update_100hz(void);
    /// update - run the loiter and wpnav controllers - should be called at 10hz
    //void update_10hz(void);
};
#endif	// AC_WPNAV_H
