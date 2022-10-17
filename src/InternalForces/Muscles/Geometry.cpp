#define BIORBD_API_EXPORTS
#include "InternalForces/Muscles/Geometry.h"

#include <rbdl/Model.h>
#include <rbdl/Kinematics.h>
#include "Utils/Error.h"
#include "Utils/Matrix.h"
#include "Utils/RotoTrans.h"
#include "RigidBody/NodeSegment.h"
#include "RigidBody/Joints.h"
#include "RigidBody/GeneralizedCoordinates.h"
#include "RigidBody/GeneralizedVelocity.h"
#include "InternalForces/WrappingObject.h"
#include "InternalForces/PathModifiers.h"
#include "InternalForces/Muscles/Characteristics.h"
#include "InternalForces/ViaPoint.h"

using namespace BIORBD_NAMESPACE;

internalforce::muscles::Geometry::Geometry() :
    m_origin(std::make_shared<utils::Vector3d>()),
    m_insertion(std::make_shared<utils::Vector3d>()),
    m_originInGlobal(std::make_shared<utils::Vector3d>
                     (utils::Vector3d::Zero())),
    m_insertionInGlobal(std::make_shared<utils::Vector3d>
                        (utils::Vector3d::Zero())),
    m_pointsInGlobal(std::make_shared<std::vector<utils::Vector3d>>()),
    m_pointsInLocal(std::make_shared<std::vector<utils::Vector3d>>()),
    m_jacobian(std::make_shared<utils::Matrix>()),
    m_G(std::make_shared<utils::Matrix>()),
    m_jacobianLength(std::make_shared<utils::Matrix>()),
    m_length(std::make_shared<utils::Scalar>(0)),
    m_muscleTendonLength(std::make_shared<utils::Scalar>(0)),
    m_velocity(std::make_shared<utils::Scalar>(0)),
    m_isGeometryComputed(std::make_shared<bool>(false)),
    m_isVelocityComputed(std::make_shared<bool>(false)),
    m_posAndJacoWereForced(std::make_shared<bool>(false))
{

}

internalforce::muscles::Geometry::Geometry(
    const utils::Vector3d &origin,
    const utils::Vector3d &insertion) :
    m_origin(std::make_shared<utils::Vector3d>(origin)),
    m_insertion(std::make_shared<utils::Vector3d>(insertion)),
    m_originInGlobal(std::make_shared<utils::Vector3d>
                     (utils::Vector3d::Zero())),
    m_insertionInGlobal(std::make_shared<utils::Vector3d>
                        (utils::Vector3d::Zero())),
    m_pointsInGlobal(std::make_shared<std::vector<utils::Vector3d>>()),
    m_pointsInLocal(std::make_shared<std::vector<utils::Vector3d>>()),
    m_jacobian(std::make_shared<utils::Matrix>()),
    m_G(std::make_shared<utils::Matrix>()),
    m_jacobianLength(std::make_shared<utils::Matrix>()),
    m_length(std::make_shared<utils::Scalar>(0)),
    m_muscleTendonLength(std::make_shared<utils::Scalar>(0)),
    m_velocity(std::make_shared<utils::Scalar>(0)),
    m_isGeometryComputed(std::make_shared<bool>(false)),
    m_isVelocityComputed(std::make_shared<bool>(false)),
    m_posAndJacoWereForced(std::make_shared<bool>(false))
{

}

internalforce::muscles::Geometry internalforce::muscles::Geometry::DeepCopy() const
{
    internalforce::muscles::Geometry copy;
    copy.DeepCopy(*this);
    return copy;
}

void internalforce::muscles::Geometry::DeepCopy(const internalforce::muscles::Geometry &other)
{
    *m_origin = other.m_origin->DeepCopy();
    *m_insertion = other.m_insertion->DeepCopy();
    *m_originInGlobal = other.m_originInGlobal->DeepCopy();
    *m_insertionInGlobal = other.m_insertionInGlobal->DeepCopy();
    m_pointsInGlobal->resize(other.m_pointsInGlobal->size());
    for (unsigned int i=0; i<other.m_pointsInGlobal->size(); ++i) {
        (*m_pointsInGlobal)[i] = (*other.m_pointsInGlobal)[i].DeepCopy();
    }
    m_pointsInLocal->resize(other.m_pointsInLocal->size());
    for (unsigned int i=0; i<other.m_pointsInLocal->size(); ++i) {
        (*m_pointsInLocal)[i] = (*other.m_pointsInLocal)[i].DeepCopy();
    }
    *m_jacobian = *other.m_jacobian;
    *m_G = *other.m_G;
    *m_jacobianLength = *other.m_jacobianLength;
    *m_length = *other.m_length;
    *m_muscleTendonLength = *other.m_muscleTendonLength;
    *m_velocity = *other.m_velocity;
    *m_isGeometryComputed = *other.m_isGeometryComputed;
    *m_isVelocityComputed = *other.m_isVelocityComputed;
    *m_posAndJacoWereForced = *other.m_posAndJacoWereForced;
}


// ------ PUBLIC FUNCTIONS ------ //
void internalforce::muscles::Geometry::updateKinematics(
    rigidbody::Joints &model,
    const rigidbody::GeneralizedCoordinates *Q,
    const rigidbody::GeneralizedVelocity *Qdot,
    int updateKin)
{
    if (*m_posAndJacoWereForced) {
        utils::Error::warning(
            false,
            "Warning, using updateKinematics overrides the"
            " previously sent position and jacobian");
        *m_posAndJacoWereForced = false;
    }

    // Make sure the model is in the right configuration
#ifdef BIORBD_USE_CASADI_MATH
    updateKin = 2;
#endif
    if (updateKin > 1) {
        model.UpdateKinematicsCustom(Q, Qdot, nullptr);
    }

    // Position of the points in space
    setMusclesPointsInGlobal(model, *Q);

    // Compute the Jacobian of the muscle points
    jacobian(model, *Q);

    // Complete the update
    _updateKinematics(Qdot);
}

void internalforce::muscles::Geometry::updateKinematics(rigidbody::Joints
        &model,
        const internalforce::muscles::Characteristics& characteristics,
        internalforce::PathModifiers &pathModifiers,
        const rigidbody::GeneralizedCoordinates *Q,
        const rigidbody::GeneralizedVelocity *Qdot,
        int updateKin)
{
    if (*m_posAndJacoWereForced) {
        utils::Error::warning(
            false, "Warning, using updateKinematics overrides the"
            " previously sent position and jacobian");
        *m_posAndJacoWereForced = false;
    }
#ifdef BIORBD_USE_CASADI_MATH
    updateKin = 2;
#endif

    // Ensure the model is in the right configuration
    if (updateKin > 1) {
        model.UpdateKinematicsCustom(Q, Qdot);
    }

    // Position of the points in space
    setMusclesPointsInGlobal(model, *Q, &pathModifiers);

    // Compute the Jacobian of the muscle points
    jacobian(model, *Q);

    // Complete the update
    _updateKinematics(Qdot, &characteristics, &pathModifiers);
}

void internalforce::muscles::Geometry::updateKinematics(
    std::vector<utils::Vector3d>& musclePointsInGlobal,
    utils::Matrix& jacoPointsInGlobal,
    const rigidbody::GeneralizedVelocity* Qdot)
{
    *m_posAndJacoWereForced = true;

    // Position of the points in space
    setMusclesPointsInGlobal(musclePointsInGlobal);

    // Compute the Jacobian of the muscle points
    jacobian(jacoPointsInGlobal);

    // Complete the update
    _updateKinematics(Qdot);
}

void internalforce::muscles::Geometry::updateKinematics(
    std::vector<utils::Vector3d>& musclePointsInGlobal,
    utils::Matrix& jacoPointsInGlobal,
    const internalforce::muscles::Characteristics& c,
    const rigidbody::GeneralizedVelocity* Qdot)
{
    *m_posAndJacoWereForced = true;

    // Position of the points in space
    setMusclesPointsInGlobal(musclePointsInGlobal);

    // Compute the Jacobian of the muscle points
    jacobian(jacoPointsInGlobal);

    // Complete the update
    _updateKinematics(Qdot, &c);
}

// Get and set the positions of the origins and insertions
void internalforce::muscles::Geometry::setOrigin(
    const utils::Vector3d &position)
{
    if (dynamic_cast<const rigidbody::NodeSegment*>(&position)) {
        *m_origin = position;
    } else {
        // Preserve the Node information
        m_origin->RigidBodyDynamics::Math::Vector3d::operator=(position);
    }
}
const utils::Vector3d& internalforce::muscles::Geometry::originInLocal() const
{
    return *m_origin;
}

void internalforce::muscles::Geometry::setInsertionInLocal(
    const utils::Vector3d &position)
{
    if (dynamic_cast<const rigidbody::NodeSegment*>(&position)) {
        *m_insertion = position;
    } else {
        // Preserve the Node information
        m_insertion->RigidBodyDynamics::Math::Vector3d::operator=(position);
    }
}
const utils::Vector3d &internalforce::muscles::Geometry::insertionInLocal()
const
{
    return *m_insertion;
}

// Position of the muscles in space
const utils::Vector3d &internalforce::muscles::Geometry::originInGlobal() const
{
    utils::Error::check(*m_isGeometryComputed,
                                "Geometry must be computed at least once before calling originInLocal()");
    return *m_originInGlobal;
}
const utils::Vector3d &internalforce::muscles::Geometry::insertionInGlobal()
const
{
    utils::Error::check(*m_isGeometryComputed,
                                "Geometry must be computed at least once before calling insertionInGlobal()");
    return *m_insertionInGlobal;
}
const std::vector<utils::Vector3d>
&internalforce::muscles::Geometry::musclesPointsInGlobal() const
{
    utils::Error::check(*m_isGeometryComputed,
                                "Geometry must be computed at least once before calling musclesPointsInGlobal()");
    return *m_pointsInGlobal;
}

// Return the length and muscular velocity
const utils::Scalar& internalforce::muscles::Geometry::length() const
{
    utils::Error::check(*m_isGeometryComputed,
                                "Geometry must be computed at least before calling length()");
    return *m_length;
}
const utils::Scalar& internalforce::muscles::Geometry::musculoTendonLength()
const
{
    utils::Error::check(*m_isGeometryComputed,
                                "Geometry must be computed at least before calling length()");
    return *m_muscleTendonLength;
}
const utils::Scalar& internalforce::muscles::Geometry::velocity() const
{
    utils::Error::check(*m_isVelocityComputed,
                                "Geometry must be computed before calling velocity()");
    return *m_velocity;
}

// Return the Jacobian
const utils::Matrix& internalforce::muscles::Geometry::jacobian() const
{
    utils::Error::check(*m_isGeometryComputed,
                                "Geometry must be computed before calling jacobian()");
    return *m_jacobian;
} // Return the last Jacobian
utils::Matrix internalforce::muscles::Geometry::jacobianOrigin() const
{
    utils::Error::check(*m_isGeometryComputed,
                                "Geometry must be computed before calling jacobianOrigin()");
    return m_jacobian->block(0,0,3,m_jacobian->cols());
}
utils::Matrix internalforce::muscles::Geometry::jacobianInsertion() const
{
    utils::Error::check(*m_isGeometryComputed,
                                "Geometry must be computed before calling jacobianInsertion()");
    return m_jacobian->block(m_jacobian->rows()-3,0,3,m_jacobian->cols());
}
utils::Matrix internalforce::muscles::Geometry::jacobian(
    unsigned int idxViaPoint) const
{
    utils::Error::check(*m_isGeometryComputed,
                                "Geometry must be computed before calling jacobian(i)");
    return m_jacobian->block(3*idxViaPoint,0,3,m_jacobian->cols());
}

const utils::Matrix &internalforce::muscles::Geometry::jacobianLength() const
{
    utils::Error::check(*m_isGeometryComputed,
                                "Geometry must be computed before calling jacobianLength()");
    return *m_jacobianLength;
}

// --------------------------------------- //

void internalforce::muscles::Geometry::_updateKinematics(
    const rigidbody::GeneralizedVelocity* Qdot,
    const internalforce::muscles::Characteristics* characteristics,
    internalforce::PathModifiers *pathModifiers)
{
    // Compute the length and velocities
    length(characteristics, pathModifiers);
    *m_isGeometryComputed = true;

    // Compute the jacobian of the lengths
    computeJacobianLength();
    if (Qdot != nullptr) {
        velocity(*Qdot);
        *m_isVelocityComputed = true;
    } else {
        *m_isVelocityComputed = false;
    }
}

const utils::Vector3d &internalforce::muscles::Geometry::originInGlobal(
    rigidbody::Joints &model,
    const rigidbody::GeneralizedCoordinates &Q)
{
    // Return the position of the marker in function of the given position
    m_originInGlobal->block(0,0,3,
                            1) = RigidBodyDynamics::CalcBodyToBaseCoordinates(model, Q,
                                    model.GetBodyId(m_origin->parent().c_str()), *m_origin,false);
    return *m_originInGlobal;
}

const utils::Vector3d &internalforce::muscles::Geometry::insertionInGlobal(
    rigidbody::Joints &model,
    const rigidbody::GeneralizedCoordinates &Q)
{
    // Return the position of the marker in function of the given position
    m_insertionInGlobal->block(0,0,3,
                               1) = RigidBodyDynamics::CalcBodyToBaseCoordinates(model, Q,
                                       model.GetBodyId(m_insertion->parent().c_str()), *m_insertion,false);
    return *m_insertionInGlobal;
}

void internalforce::muscles::Geometry::setMusclesPointsInGlobal(
    std::vector<utils::Vector3d> &ptsInGlobal)
{
    utils::Error::check(ptsInGlobal.size() >= 2,
                                "ptsInGlobal must at least have an origin and an insertion");
    m_pointsInLocal->clear(); // In this mode, we don't need the local, because the Jacobian of the points has to be given as well
    *m_pointsInGlobal = ptsInGlobal;
}

void internalforce::muscles::Geometry::setMusclesPointsInGlobal(
    rigidbody::Joints &model,
    const rigidbody::GeneralizedCoordinates &Q,
    internalforce::PathModifiers *pathModifiers)
{
    // Output varible (reset to zero)
    m_pointsInLocal->clear();
    m_pointsInGlobal->clear();

    // Do not apply on wrapping objects
    if (pathModifiers->nbWraps()!=0) {
        // CHECK TO MODIFY BEFOR GOING FORWARD WITH PROJECTS
        utils::Error::check(pathModifiers->nbVia() == 0,
                                    "Cannot mix wrapping and via points yet") ;
        utils::Error::check(pathModifiers->nbWraps() < 2,
                                    "Cannot compute more than one wrapping yet");

        // Get the matrix of Rt of the wrap
        internalforce::WrappingObject& w =
            static_cast<internalforce::WrappingObject&>(pathModifiers->object(0));
        const utils::RotoTrans& RT = w.RT(model,Q);

        // Alias
        const utils::Vector3d& po_mus = originInGlobal(model,
                                                Q);  // Origin on bone
        const utils::Vector3d& pi_mus = insertionInGlobal(model,
                                                Q); // Insertion on bone

        utils::Vector3d pi_wrap(0, 0,
                                        0); // point on the wrapping related to insertion
        utils::Vector3d po_wrap(0, 0,
                                        0); // point on the wrapping related to origin

        utils::Scalar a; // Force the computation of the length
        w.wrapPoints(RT,po_mus,pi_mus,po_wrap, pi_wrap, &a);

        // Store the points in local
        m_pointsInLocal->push_back(originInLocal());
        m_pointsInLocal->push_back(
            utils::Vector3d(RigidBodyDynamics::CalcBodyToBaseCoordinates(
                                        model, Q, model.GetBodyId(w.parent().c_str()),po_wrap, false),
                                    "wrap_o", w.parent()));
        m_pointsInLocal->push_back(
            utils::Vector3d(RigidBodyDynamics::CalcBodyToBaseCoordinates(
                                        model, Q, model.GetBodyId(w.parent().c_str()),pi_wrap, false),
                                    "wrap_i", w.parent()));
        m_pointsInLocal->push_back(insertionInLocal());

        // Store the points in global
        m_pointsInGlobal->push_back(po_mus);
        m_pointsInGlobal->push_back(po_wrap);
        m_pointsInGlobal->push_back(pi_wrap);
        m_pointsInGlobal->push_back(pi_mus);

    }

    else if (pathModifiers->nbObjects()!=0
             && pathModifiers->object(0).typeOfNode() == utils::NODE_TYPE::VIA_POINT) {
        m_pointsInLocal->push_back(originInLocal());
        m_pointsInGlobal->push_back(originInGlobal(model, Q));
        for (unsigned int i=0; i<pathModifiers->nbObjects(); ++i) {
            const internalforce::ViaPoint& node(static_cast<internalforce::ViaPoint&>
                                                  (pathModifiers->object(i)));
            m_pointsInLocal->push_back(node);
            m_pointsInGlobal->push_back(RigidBodyDynamics::CalcBodyToBaseCoordinates(model,
                                        Q,
                                        model.GetBodyId(node.parent().c_str()), node, false));
        }
        m_pointsInLocal->push_back(insertionInLocal());
        m_pointsInGlobal->push_back(insertionInGlobal(model,Q));

    } else if (pathModifiers->nbObjects()==0) {
        m_pointsInLocal->push_back(originInLocal());
        m_pointsInLocal->push_back(insertionInLocal());
        m_pointsInGlobal->push_back(originInGlobal(model, Q));
        m_pointsInGlobal->push_back(insertionInGlobal(model,Q));
    } else {
        utils::Error::raise("Length for this type of object was not implemented");
    }

    // Set the dimension of jacobian
    setJacobianDimension(model);
}

const utils::Scalar& internalforce::muscles::Geometry::length(
    const internalforce::muscles::Characteristics *characteristics,
    internalforce::PathModifiers *pathModifiers)
{
    *m_muscleTendonLength = 0;

    // because we can't combine, test the first (0) will let us know all the types if more than one
    if (pathModifiers != nullptr && pathModifiers->nbWraps()!=0) {
        utils::Error::check(pathModifiers->nbVia() == 0,
                                    "Cannot mix wrapping and via points yet" ) ;
        utils::Error::check(pathModifiers->nbWraps() < 2,
                                    "Cannot compute more than one wrapping yet");

        utils::Vector3d pi_wrap(0, 0,
                                        0); // point on the wrapping related to insertion
        utils::Vector3d po_wrap(0, 0,
                                        0); // point on the wrapping related to origin
        utils::Scalar lengthWrap(0);
        static_cast<internalforce::WrappingObject&>(
            pathModifiers->object(0)).wrapPoints(po_wrap, pi_wrap, &lengthWrap);
        *m_muscleTendonLength = ((*m_pointsInGlobal)[0] - po_wrap).norm()
                                + // length before the wrap
                                lengthWrap                 + // length on the wrap
                                ((*m_pointsInGlobal)[3] - pi_wrap).norm();   // length after the wrap

    } else {
        for (unsigned int i=0; i<m_pointsInGlobal->size()-1; ++i) {
            *m_muscleTendonLength += ((*m_pointsInGlobal)[i+1] -
                                      (*m_pointsInGlobal)[i]).norm();
        }
    }

    *m_length = (*m_muscleTendonLength - characteristics->tendonSlackLength())
                /
                std::cos(characteristics->pennationAngle());

    return *m_length;
}

const utils::Scalar& internalforce::muscles::Geometry::velocity(
    const rigidbody::GeneralizedVelocity &Qdot)
{
    // Compute the velocity of the muscular elongation
    *m_velocity = (jacobianLength()*Qdot)[0];
    return *m_velocity;
}

void internalforce::muscles::Geometry::setJacobianDimension(rigidbody::Joints
        &model)
{
    *m_jacobian = utils::Matrix::Zero(static_cast<unsigned int>
                  (m_pointsInLocal->size()*3), model.dof_count);
    *m_G = utils::Matrix::Zero(3, model.dof_count);
}

void internalforce::muscles::Geometry::jacobian(const utils::Matrix &jaco)
{
    utils::Error::check(jaco.rows()/3 == static_cast<int>
                                (m_pointsInGlobal->size()), "Jacobian is the wrong size");
    *m_jacobian = jaco;
}

void internalforce::muscles::Geometry::jacobian(
    rigidbody::Joints &model,
    const rigidbody::GeneralizedCoordinates &Q)
{
    for (unsigned int i=0; i<m_pointsInLocal->size(); ++i) {
        m_G->setZero();
        RigidBodyDynamics::CalcPointJacobian(model, Q,
                                             model.GetBodyId((*m_pointsInLocal)[i].parent().c_str()),
                                             (*m_pointsInLocal)[i], *m_G, false); // False for speed
        m_jacobian->block(3*i,0,3,model.dof_count) = *m_G;
    }
}

void internalforce::muscles::Geometry::computeJacobianLength()
{
    *m_jacobianLength = utils::Matrix::Zero(1, m_jacobian->cols());

    // jacobian approximates as if there were no wrapping object
    const std::vector<utils::Vector3d>& p = *m_pointsInGlobal;
    for (unsigned int i=0; i<p.size()-1 ; ++i) {
        *m_jacobianLength += (( p[i+1] - p[i] ).transpose() * (jacobian(i+1) - jacobian(
                                  i)))
                             /
                             ( p[i+1] - p[i] ).norm();
    }
}
