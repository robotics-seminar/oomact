#include "aslam/calibration/ros/RosInputProvider.h"

#include <memory>

#include <gtest/gtest.h>
#include <rosbag/bag.h>

#include <aslam/calibration/calibrator/CalibratorI.h>
#include <aslam/calibration/model/FrameGraphModel.h>
#include <aslam/calibration/model/PoseTrajectory.h>
#include <aslam/calibration/model/sensors/PoseSensor.h>
#include <aslam/calibration/model/sensors/Velodyne.h>
#include <aslam/calibration/test/TestData.h>
#include <aslam/calibration/tools/SmartPointerTools.h>


using namespace aslam;
using namespace aslam::calibration;
using namespace aslam::calibration::ros;


TEST(RosInputProviderSuite, testEasy) {
  const std::string testBagPath = "oomact_pointclouds_ros/velodyne-sim.bag";
  OOMACT_SKIP_IF_TESTDATA_UNAVAILABLE(testBagPath);

  // Create configuration
  auto vs = ValueStoreRef::fromFile("velodyne-velodyne.xml");
//   String(
//      "model{"
//        "Gravity{used=false}, frames=body:world\n" // TODO B make false default but have a sanity check in IMU or who ever uses it!
//        "a{frame=body,targetFrame=world,rotation/used=false,translation{used=true,x=0,y=5,z=0},delay/used=false,topic=velodyne_points}"
//        "b{frame=body,targetFrame=world,rotation/used=false,translation{used=false},delay/used=false,topic=velodyne_points}" // uses the same topic as a!
//      "}"
//      "calibrator{"
//        "verbose=true\n"
//        "timeBaseSensor=a\n"
//      "}"
//    );

  FrameGraphModel m(vs.getChild("oomact/model"));
  Velodyne psA(m, "sensors/a"), psB(m, "sensors/b");
  m.addModulesAndInit(psA, psB);

  // Create local pseudo shared pointer to model and create a batchCalibrator for it
  auto spModel = to_local_shared_ptr(m);
  auto c = createBatchCalibrator(vs.getChild("oomact/calibrator"), spModel);

  RosInputProvider(spModel).feedBag(test::getTestDataPath(testBagPath), *c); // Feed the test.bag

  EXPECT_NEAR(5.0, psA.getTranslationToParent()[1], 0.0001); // check the y position of the sensor a BEFORE calibration

  c->calibrate();

  EXPECT_NEAR(0.0, psA.getTranslationToParent()[1], 0.0001); // it should be zero now because it is receiving the same data as b, which has zero transformation and doesn't get calibrated.
}
