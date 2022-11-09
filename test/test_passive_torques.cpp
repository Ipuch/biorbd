#include <iostream>
#include <gtest/gtest.h>
#include <rbdl/rbdl_math.h>
#include <rbdl/Dynamics.h>


#include "BiorbdModel.h"
#include "biorbdConfig.h"
#include "RigidBody/GeneralizedCoordinates.h"
#include "RigidBody/GeneralizedVelocity.h"
#include "RigidBody/GeneralizedTorque.h"
#include "InternalForces/all.h"
#include "InternalForces/PassiveTorques/all.h"

using namespace BIORBD_NAMESPACE;

static std::string modelPathForGeneralTesting("models/arm26WithActuators.bioMod");
static std::string modelPathWithoutPassiveTorques("models/arm26.bioMod");


static double requiredPrecision(1e-10);

TEST(FileIO, openModelWithPassiveTorques)
{
    EXPECT_NO_THROW(Model model(modelPathForGeneralTesting));
    EXPECT_NO_THROW(Model model(modelPathWithoutPassiveTorques));
}

TEST(PassiveTorqueConstant, passiveTorque)
{
    internal_forces::passive_torques::PassiveTorqueConstant const_torque_act(150, 0);
    SCALAR_TO_DOUBLE(torqueConstantVal, const_torque_act.passiveTorque())
    EXPECT_NEAR(torqueConstantVal, 150, requiredPrecision);
}

TEST(PassiveTorqueLinear, passiveTorque)
{
    // A model is loaded so Q can be > 0 in size, it is not used otherwise
    Model model(modelPathForGeneralTesting);
    DECLARE_GENERALIZED_COORDINATES(Q, model);

    std::vector<double> val = {1.1};
    FILL_VECTOR(Q, val);
    double torqueLinearExpected(88.025357464390567);
    internal_forces::passive_torques::PassiveTorqueLinear linear_torque_act(1, 25, 1);
    CALL_BIORBD_FUNCTION_1ARG(torqueLinearVal, linear_torque_act, passiveTorque, Q);
#ifdef BIORBD_USE_CASADI_MATH
    EXPECT_NEAR(static_cast<double>(torqueLinearVal(0, 0)), torqueLinearExpected,
                requiredPrecision);
#else
    EXPECT_NEAR(torqueLinearVal, torqueLinearExpected, requiredPrecision);
#endif
}

TEST(PassiveTorques, NbPassiveTorques)
{
    {
        Model model(modelPathForGeneralTesting);
        size_t val(model.nbPassiveTorques());
        EXPECT_NEAR(val, 2, requiredPrecision);
    }
    {
        Model model(modelPathWithoutPassiveTorques);
        size_t val(model.nbPassiveTorques());
        EXPECT_NEAR(val, 0, requiredPrecision);
    }
}

TEST(PassiveTorques, jointTorqueFromAllTypesOfPassiveTorque)
{
    Model model(modelPathForGeneralTesting);
    DECLARE_GENERALIZED_COORDINATES(Q, model);
    DECLARE_GENERALIZED_VELOCITY(QDot, model);
    std::vector<double> Q_val(model.nbQ());
    for (size_t i=0; i<Q_val.size(); ++i) {
        Q_val[i] = 1.1;
    }
    FILL_VECTOR(Q, Q_val);

    std::vector<double> QDot_val(model.nbQdot());
    for (size_t i=0; i<QDot_val.size(); ++i) {
        QDot_val[i] = 1.1;
    }
    FILL_VECTOR(QDot, QDot_val);

    std::vector<double> torqueExpected = {10, 90};

    CALL_BIORBD_FUNCTION_2ARGS(tau, model, passiveJointTorque, Q, QDot);
    EXPECT_NEAR(tau.size(), 2, requiredPrecision);
    for (size_t i=0; i<model.nbGeneralizedTorque(); ++i) {
        EXPECT_NEAR(static_cast<double>(tau(i, 0)), torqueExpected[i], requiredPrecision);
    }
}
