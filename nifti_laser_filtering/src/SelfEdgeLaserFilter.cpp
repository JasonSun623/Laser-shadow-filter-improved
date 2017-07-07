#include <math.h>

#include "nifti_laser_filtering/SelfEdgeLaserFilter.h"

#include "pluginlib/class_list_macros.h"

namespace nifti_laser_filtering {

    SelfEdgeLaserFilter::~SelfEdgeLaserFilter() {
    }

    bool SelfEdgeLaserFilter::configure() {
      if (!filters::FilterBase<sensor_msgs::LaserScan>::getParam("max_edge_distance", max_edge_dist)) {
          ROS_WARN("SelfEdgeLaserFilter was not given max_edge_distance, assuming 0.5 meters.");
          max_edge_dist = 0.5;
      } else {
          ROS_DEBUG("SelfEdgeLaserFilter: found param max_edge_distance: %.5f meters", max_edge_dist);
      }

      double min_filter_angle;
      if (!filters::FilterBase<sensor_msgs::LaserScan>::getParam("min_filter_angle", min_filter_angle)) {
          ROS_WARN("SelfEdgeLaserFilter was not given min_filter_angle, assuming 1.466 rad.");
          min_filter_angle = 1.466;
      } else {
          ROS_DEBUG("SelfEdgeLaserFilter: found param min_filter_angle: %.5f rad", min_filter_angle);
      }
      cos_max = cos(min_filter_angle);

      double max_filter_angle;
      if (!filters::FilterBase<sensor_msgs::LaserScan>::getParam("max_filter_angle", max_filter_angle)) {
          ROS_WARN("SelfEdgeLaserFilter was not given max_filter_angle, assuming 2.932 rad.");
          max_filter_angle = 2.932;
      } else {
          ROS_DEBUG("SelfEdgeLaserFilter: found param max_filter_angle: %.5f rad", max_filter_angle);
      }
      cos_min = cos(max_filter_angle);

      if (!filters::FilterBase<sensor_msgs::LaserScan>::getParam("delta_threshold", delta_threshold)) {
          ROS_WARN("SelfEdgeLaserFilter was not given delta_threshold, assuming 0.01 meters.");
          delta_threshold = 0.01;
      } else {
          ROS_DEBUG("SelfEdgeLaserFilter: found param delta_threshold: %.5f rad", delta_threshold);
      }

        ROS_INFO("SelfEdgeLaserFilter: Successfully configured.");
        return true;
    }

    bool SelfEdgeLaserFilter::update(const sensor_msgs::LaserScan &input_scan, sensor_msgs::LaserScan &filtered_scan) {
        /*const float max_edge_dist = 0.5;
        const float cos_min = -0.98;
        const float cos_max = 0.10;
        const float delta_threshold = 0.01;*/

        const float cos_gamma = cos(input_scan.angle_increment);
        const float cos_2gamma = cos(2 * input_scan.angle_increment);

        float a1, a2, a3, r1, r2, r3, ra, rb, dr, dr2, cos_edge;
        int num_filtered_points = 0;
        int sgn;

        filtered_scan = input_scan;
        for (unsigned int i = 0; i < filtered_scan.ranges.size(); i++) {
            r1 = r2;
            r2 = r3;
            r3 = filtered_scan.ranges[i];
            if (i < 2) { //get at least three points
              continue;
            }

            if ((r1 != r1) || (r2 != r2) || (r3 != r3)) { //some points already filtered
              continue;
            }

            if (r2 > max_edge_dist) { //edge too far
              continue;
            }

            if (r2 > r1 || r2 > r3) { //edge not convex
              continue;
            }

            a1 = r1*r1 + r2*r2 - 2*r1*r2*cos_gamma;
            a2 = r2*r2 + r3*r3 - 2*r1*r2*cos_gamma;
            a3 = r1*r1 + r3*r3 - 2*r1*r2*cos_gamma;

            cos_edge = (a3 - a1 - a2) / (2 * sqrt(a1*a2));

            if ((cos_edge < cos_max) || (cos_edge > cos_min)) {
              int j = i;
              //Let's remove the trail.
              ra = r2;
              if (a1 < a3) { //decide which way the trail continues
                sgn = -1;
                j -= 2;
              } else {
                sgn = 1;
              }

              while (j >= 0 && j < filtered_scan.ranges.size()) {
                rb = ra;
                ra = filtered_scan.ranges[j];
                dr2 = rb - ra;
                if (fabs(dr2) < delta_threshold) {
                  dr = dr2;
                  filtered_scan.ranges[j] = std::numeric_limits<float>::quiet_NaN();
                  num_filtered_points += 1;
                } else {
                  break;
                }
                j += sgn;
              }
              //filtered_scan.ranges[i] = std::numeric_limits<float>::quiet_NaN();
              //num_filtered_points += 1;
            }
        }

        ROS_DEBUG("Self edge shadow filtering removed %u points.", num_filtered_points);

        return true;
    }
}

PLUGINLIB_EXPORT_CLASS(nifti_laser_filtering::SelfEdgeLaserFilter, filters::FilterBase<sensor_msgs::LaserScan>);

