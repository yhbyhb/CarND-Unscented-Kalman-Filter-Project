#include "ukf.h"
#include "tools.h"
#include "Eigen/Dense"
#include <iostream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/**
 * Initializes Unscented Kalman filter
 * This is scaffolding, do not modify
 */
UKF::UKF() {
  // if this is false, laser measurements will be ignored (except during init)
  use_laser_ = true;

  // if this is false, radar measurements will be ignored (except during init)
  use_radar_ = true;

  // initial state vector
  x_ = VectorXd(5);

  // initial covariance matrix
  P_ = MatrixXd(5, 5);

  // Process noise standard deviation longitudinal acceleration in m/s^2
  std_a_ = 2;

  // Process noise standard deviation yaw acceleration in rad/s^2
  std_yawdd_ = 0.785;
  
  //DO NOT MODIFY measurement noise values below these are provided by the sensor manufacturer.
  // Laser measurement noise standard deviation position1 in m
  std_laspx_ = 0.15;

  // Laser measurement noise standard deviation position2 in m
  std_laspy_ = 0.15;

  // Radar measurement noise standard deviation radius in m
  std_radr_ = 0.3;

  // Radar measurement noise standard deviation angle in rad
  std_radphi_ = 0.03;

  // Radar measurement noise standard deviation radius change in m/s
  std_radrd_ = 0.3;
  //DO NOT MODIFY measurement noise values above these are provided by the sensor manufacturer.
  
  /**
  TODO:

  Complete the initialization. See ukf.h for other member properties.

  Hint: one or more values initialized above might be wildly off...
  */
  
  //set state dimension
  n_x_ = 5;
  
  //set augmented dimension
  n_aug_ = 7;
  
  //set measurement dimension, radar can measure r, phi, and r_dot
  Xsig_pred_ = MatrixXd(n_x_, 2 * n_aug_ + 1);
  
  weights_= VectorXd(2 * n_aug_ +1);
}

UKF::~UKF() {}

/**
 * @param {MeasurementPackage} meas_package The latest measurement data of
 * either radar or laser.
 */
void UKF::ProcessMeasurement(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Make sure you switch between lidar and radar
  measurements.
  */
  
  /*****************************************************************************
   *  Initialization
   ****************************************************************************/
  if (!is_initialized_) {
    // first measurement
    cout << "UKF: " << endl;

    // init matrix
    P_ << 1, 0, 0, 0, 0,
          0, 1, 0, 0, 0,
          0, 0, 1, 0, 0,
          0, 0, 0, 1, 0,
          0, 0, 0, 0, 1;

    x_ << 0, 0, 0, 0, 0;
    if (meas_package.sensor_type_ == MeasurementPackage::RADAR) {
      /**
       Convert radar from polar to cartesian coordinates and initialize state.
       */
      double rho = meas_package.raw_measurements_[0];
      double phi =  meas_package.raw_measurements_[1];
      x_ << rho * cos(phi), rho * sin(phi), 0, 0, 0;
    }
    else if (meas_package.sensor_type_ == MeasurementPackage::LASER) {
      /**
       Initialize state.
       */
      x_ << meas_package.raw_measurements_[0], meas_package.raw_measurements_[1], 0, 0, 0;
    }
    
    time_us_ = meas_package.timestamp_;
    is_initialized_ = true;
  }
  
  /*****************************************************************************
   *  Prediction
   ****************************************************************************/
  float dt = (meas_package.timestamp_ - time_us_) / 1000000.0;
  time_us_ = meas_package.timestamp_;
  
  Prediction(dt);
  
  /*****************************************************************************
   *  Update
   ****************************************************************************/
  if (meas_package.sensor_type_ == MeasurementPackage::RADAR) {
    // Radar updates
    UpdateRadar(meas_package);
  }
  else if (meas_package.sensor_type_ == MeasurementPackage::LASER) {
    // Laser updates
    UpdateLidar(meas_package);
  }
}

/**
 * Predicts sigma points, the state, and the state covariance matrix.
 * @param {double} delta_t the change in time (in seconds) between the last
 * measurement and this one.
 */
void UKF::Prediction(double delta_t) {
  /**
  TODO:

  Complete this function! Estimate the object's location. Modify the state
  vector, x_. Predict sigma points, the state, and the state covariance matrix.
  */

  //Tool for normalizing angle
  Tools tools;

  /// Generating sigma points
  
  //calculate square root of P
  MatrixXd A = P_.llt().matrixL();
  
  //define spreading parameter
  lambda_ = 3 - n_x_;
  double sqrt_lambda_plus_n_x = sqrt(lambda_ + n_x_);
  
  //create sigma point matrix
  MatrixXd Xsig = MatrixXd(n_x_, 2 * n_x_ + 1);
  Xsig.col(0) = x_;
  for (int i = 0; i < n_x_; i++)
  {
    Xsig.col(i+1)        = x_ + sqrt_lambda_plus_n_x * A.col(i);
    Xsig.col(i+1 + n_x_) = x_ - sqrt_lambda_plus_n_x * A.col(i);
  }
  
  /// Augmentation
  
  //create augmented mean vector
  VectorXd x_aug = VectorXd(n_aug_);
  //create augmented mean state
  x_aug.head(n_x_) = x_;
  x_aug(5) = 0;
  x_aug(6) = 0;
  
  //create augmented covariance matrix
  MatrixXd Q = MatrixXd(2, 2);
  Q << std_a_ * std_a_, 0,
       0,               std_yawdd_ * std_yawdd_;
  
  //create augmented state covariance
  MatrixXd P_aug = MatrixXd(n_aug_, n_aug_);
  P_aug.fill(0.0);
  P_aug.topLeftCorner(n_x_, n_x_) = P_;
  P_aug.block<2,2>(5,5) = Q;
  
  //create square root matrix
  MatrixXd L = P_aug.llt().matrixL();
  
  //define spreading parameter
  lambda_ = 3 - n_aug_;
  double sqrt_lambda_plus_n_aug = sqrt(lambda_ + n_aug_);
  
  //create augmented sigma points
  MatrixXd Xsig_aug = MatrixXd(n_aug_, 2 * n_aug_ + 1);
  Xsig_aug.col(0) = x_aug;
  for (int i = 0; i < n_aug_; i++)
  {
    Xsig_aug.col(i+1)          = x_aug + sqrt_lambda_plus_n_aug * L.col(i);
    Xsig_aug.col(i+1 + n_aug_) = x_aug - sqrt_lambda_plus_n_aug * L.col(i);
  }
  
  /// Sigma point prediction
  
  //predict sigma points
  for (int i = 0; i < 2 * n_aug_ + 1; i++)
  {
    //extract values for better readability
    double p_x = Xsig_aug(0,i);
    double p_y = Xsig_aug(1,i);
    double v = Xsig_aug(2,i);
    double yaw = Xsig_aug(3,i);
    double yawd = Xsig_aug(4,i);
    double nu_a = Xsig_aug(5,i);
    double nu_yawdd = Xsig_aug(6,i);
    
    //predicted state values
    double px_p, py_p;
    
    //avoid division by zero
    if (fabs(yawd) > 0.001) {
      px_p = p_x + v / yawd * (sin(yaw + yawd * delta_t) - sin(yaw));
      py_p = p_y + v / yawd * (cos(yaw) - cos(yaw + yawd * delta_t));
    }
    else {
      px_p = p_x + v * delta_t * cos(yaw);
      py_p = p_y + v * delta_t * sin(yaw);
    }
    
    double v_p = v;
    double yaw_p = yaw + yawd * delta_t;
    double yawd_p = yawd;
    
    //add noise
    px_p = px_p + 0.5 * nu_a * delta_t * delta_t * cos(yaw);
    py_p = py_p + 0.5 * nu_a * delta_t * delta_t * sin(yaw);
    v_p = v_p + nu_a * delta_t;
    
    yaw_p = yaw_p + 0.5 * nu_yawdd * delta_t * delta_t;
    yawd_p = yawd_p + nu_yawdd * delta_t;
    
    //write predicted sigma point into right column
    Xsig_pred_(0,i) = px_p;
    Xsig_pred_(1,i) = py_p;
    Xsig_pred_(2,i) = v_p;
    Xsig_pred_(3,i) = yaw_p;
    Xsig_pred_(4,i) = yawd_p;
  }
  
  // Predicted mean and covariance
  //set weights
  weights_(0) = lambda_ / (double)(lambda_ + n_aug_);
  double weight = 1 / ((double)2 * (lambda_ + n_aug_));
  for (int i = 1; i < 2 * n_aug_ + 1; i++)
  {
    weights_(i) = weight;
  }
  
  //predict state mean
  x_.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++)
  {
    x_ += weights_(i) * Xsig_pred_.col(i);
  }
  
  //predict state covariance matrix
  P_.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++)
  {
    VectorXd diff = (Xsig_pred_.col(i) - x_);
    
    //angle normalization
    tools.NormalizeAngle(diff(3));
    
    P_ += weights_(i) * diff * diff.transpose();
  }
}

/**
 * Updates the state and the state covariance matrix using a laser measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateLidar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use lidar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the lidar NIS.
  */
 
  // Predict lidar measurement
  int n_z = 2;
  double px = meas_package.raw_measurements_[0];
  double py =  meas_package.raw_measurements_[1];
  VectorXd z = VectorXd(n_z);
  z << px, py;
 
  //create matrix for sigma points in measurement space
  MatrixXd Zsig = MatrixXd(n_z, 2 * n_aug_ + 1);
  
  //transform sigma points into measurement space
  for (int i = 0; i < 2 * n_aug_ + 1; ++i)
  {
    VectorXd sig = Xsig_pred_.col(i);
    double px = sig(0);
    double py = sig(1);
 
    Zsig.col(i) << px, py;
  }
  
  //mean predicted measurement
  VectorXd z_pred = VectorXd(n_z);
  z_pred.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; ++i)
  {
    z_pred += weights_(i) * Zsig.col(i);
  }
  
  //measurement covariance matrix S
  MatrixXd S = MatrixXd(n_z,n_z);
  S.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; ++i)
  {
    VectorXd diff = Zsig.col(i) - z_pred;
    S += weights_(i) * diff * diff.transpose();
  }
  
  //add measurement noise covariance matrix
  MatrixXd R = MatrixXd(n_z, n_z);
  R << std_laspx_ * std_laspx_, 0,
       0,                       std_laspy_ * std_laspy_;
  S += R;
  
  /// update
  
  //create matrix for cross correlation Tc
  MatrixXd Tc = MatrixXd(n_x_, n_z);
  
  //calculate cross correlation matrix
  Tc.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points
    //residual
    VectorXd z_diff = Zsig.col(i) - z_pred;
    
    // state difference
    VectorXd x_diff = Xsig_pred_.col(i) - x_;
    
    Tc += weights_(i) * x_diff * z_diff.transpose();
  }
  
  //Kalman gain K;
  MatrixXd S_inverse = S.inverse();
  MatrixXd K = Tc * S_inverse;
  
  //residual
  VectorXd z_diff = z - z_pred;
  
  //update state mean and covariance matrix
  x_ = x_ + K * z_diff;
  P_ = P_ - K * S * K.transpose();
  
  //NIS
  // std::cout << "Lidar NIS : " << z_diff.transpose() * S_inverse * z_diff << std::endl;
}

/**
 * Updates the state and the state covariance matrix using a radar measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateRadar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use radar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the radar NIS.
  */
  //Tool for normalizing angle
  Tools tools;

  // Predict radar measurement
  int n_z = 3;
  double rho = meas_package.raw_measurements_[0];
  double phi =  meas_package.raw_measurements_[1];
  double rho_dot =  meas_package.raw_measurements_[2];
  VectorXd z = VectorXd(n_z);
  z << rho, phi, rho_dot;
  
  //create matrix for sigma points in measurement space
  MatrixXd Zsig = MatrixXd(n_z, 2 * n_aug_ + 1);

  //transform sigma points into measurement space
  for (int i = 0; i < 2 * n_aug_ + 1; ++i)
  {
    // extract values for better readibility
    VectorXd sig = Xsig_pred_.col(i);
    double px = sig(0);
    double py = sig(1);
    double v = sig(2);
    double yaw = sig(3);
    
    double rho = sqrt(px * px + py * py);
    double phi = atan2(py, px);
    double rho_dot = (px * cos(yaw) * v + py * sin(yaw) * v) / rho;
    
    // measurement model
    Zsig.col(i) << rho, phi, rho_dot;
  }
  
  //mean predicted measurement
  VectorXd z_pred = VectorXd(n_z);
  z_pred.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; ++i)
  {
    z_pred += weights_(i) * Zsig.col(i);
  }
  
  //measurement covariance matrix S
  MatrixXd S = MatrixXd(n_z,n_z);
  S.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; ++i)
  {
    //residual
    VectorXd diff = Zsig.col(i) - z_pred;
    
    //angle normalization
    tools.NormalizeAngle(diff(1));
    
    S += weights_(i) * diff * diff.transpose();
  }
  
  //add measurement noise covariance matrix
  MatrixXd R = MatrixXd(n_z, n_z);
  R << std_radr_ * std_radr_, 0,                         0,
       0,                     std_radphi_ * std_radphi_, 0,
       0,                     0,                         std_radrd_ * std_radrd_;
  S += R;
  
  /// update
  
  //create matrix for cross correlation Tc
  MatrixXd Tc = MatrixXd(n_x_, n_z);
  
  //calculate cross correlation matrix
  Tc.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points
    //residual
    VectorXd z_diff = Zsig.col(i) - z_pred;
    //angle normalization
    tools.NormalizeAngle(z_diff(1));
    
    // state difference
    VectorXd x_diff = Xsig_pred_.col(i) - x_;
    //angle normalization
    tools.NormalizeAngle(x_diff(3));
    
    Tc += weights_(i) * x_diff * z_diff.transpose();
  }
  
  //Kalman gain K;
  MatrixXd S_inverse = S.inverse();
  MatrixXd K = Tc * S_inverse;
  
  //residual
  VectorXd z_diff = z - z_pred;
  
  //angle normalization
  tools.NormalizeAngle(z_diff(1));
  
  //update state mean and covariance matrix
  x_ = x_ + K * z_diff;
  P_ = P_ - K * S * K.transpose();

  //NIS
  // std::cout << "Radar NIS : " << z_diff.transpose() * S_inverse * z_diff << std::endl;
}
