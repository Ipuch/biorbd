#define BIORBD_API_EXPORTS
#include "InternalForces/Ligaments/Characteristics.h"

using namespace BIORBD_NAMESPACE;

internal_forces::ligaments::Characteristics::Characteristics() :
    m_ligamentSlackLength(std::make_shared<utils::Scalar>(0)),
    m_cste_damping(std::make_shared<utils::Scalar>(0)),
    m_cste_maxShorteningSpeed(std::make_shared<utils::Scalar>(1))
{

}

internal_forces::ligaments::Characteristics::Characteristics(
    const internal_forces::ligaments::Characteristics &other) :
    m_ligamentSlackLength(other.m_ligamentSlackLength),
    m_cste_damping(other.m_cste_damping),
    m_cste_maxShorteningSpeed(other.m_cste_maxShorteningSpeed)
{

}

internal_forces::ligaments::Characteristics::Characteristics(
    const utils::Scalar& ligamentSlackLength,
    const utils::Scalar& cste_damping,
    const utils::Scalar& cste_maxShorteningSpeed):
    m_ligamentSlackLength(std::make_shared<utils::Scalar>(ligamentSlackLength)),
    m_cste_damping(std::make_shared<utils::Scalar>(cste_damping)),
    m_cste_maxShorteningSpeed(std::make_shared<utils::Scalar>(cste_maxShorteningSpeed))

{

}

internal_forces::ligaments::Characteristics::~Characteristics()
{

}

internal_forces::ligaments::Characteristics internal_forces::ligaments::Characteristics::DeepCopy()
const
{
    internal_forces::ligaments::Characteristics copy;
    copy.DeepCopy(*this);
    return copy;
}

void internal_forces::ligaments::Characteristics::DeepCopy(
    const internal_forces::ligaments::Characteristics &other)
{
    *m_ligamentSlackLength = *other.m_ligamentSlackLength;
    *m_cste_damping = *other.m_cste_damping;
    *m_cste_maxShorteningSpeed = *other.m_cste_maxShorteningSpeed;
}

void internal_forces::ligaments::Characteristics::setLigamentSlackLength(
    const utils::Scalar& val)
{
    *m_ligamentSlackLength = val;
}
const utils::Scalar& internal_forces::ligaments::Characteristics::ligamentSlackLength() const
{
    return *m_ligamentSlackLength;
}

void internal_forces::ligaments::Characteristics::setMaxShorteningSpeed(
    const utils::Scalar& val)
{
    *m_cste_maxShorteningSpeed= val;
}
const utils::Scalar& internal_forces::ligaments::Characteristics::maxShorteningSpeed() const
{
    return *m_cste_maxShorteningSpeed;
}

void internal_forces::ligaments::Characteristics::setDampingParam(
    const utils::Scalar& val)
{
    *m_cste_damping = val;
}
const utils::Scalar& internal_forces::ligaments::Characteristics::dampingParam() const
{
    return *m_cste_damping;
}
