//
// Created by Devilast on 6/27/2021.
//
#pragma once

#include <glm/mat4x4.hpp>


namespace snv
{

class Transform
{
public:
    Transform() noexcept;

    const glm::mat4x4& GetTransform() const noexcept;

private:
    glm::mat4x4 m_transform;
};

} // namespace snv
