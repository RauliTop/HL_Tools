#pragma once

#include <cassert>

#include <entt/entity/fwd.hpp>
#include <entt/entity/registry.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "utility/math/CoordinateSystem.hpp"

namespace game::components
{
/**
*	@brief Represents the translation of an entity in local space
*/
struct Translation final
{
	glm::vec3 Value{0.f};
};

/**
*	@brief Represents the rotation of an entity in local space
*/
struct Rotation final
{
	glm::quat Value{0.f, 0.f, 0.f, 0.f};
};

/**
*	@brief Optional component to apply Euler angles to the @see Rotation component
*/
struct RotationEulerXYZ final
{
	glm::vec3 Value{0.f};
};

/**
*	@brief Represents the uniform scale of an entity
*/
struct Scale final
{
	float Value = 1.f;
};

/**
*	@brief Represents the matrix that translates an entity's position from local space to the parent's coordinate system
*/
struct LocalToParent final
{
	glm::mat4x4 Value;
};

/**
*	@brief Represents the matrix that translates an entity's position from local space to the world
*/
struct LocalToWorld final
{
	glm::mat4x4 Value;

	glm::vec3 GetPosition() const
	{
		return Value[3];
	}

	glm::quat GetRotation() const
	{
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 position;
		glm::vec3 skew;
		glm::vec4 perspective;

		glm::decompose(Value, scale, rotation, position, skew, perspective);

		return rotation;
	}

	glm::vec3 GetForward() const
	{
		return math::GetForwardVector(Value);
	}

	glm::vec3 GetRight() const
	{
		return math::GetRightVector(Value);
	}

	glm::vec3 GetUp() const
	{
		return math::GetUpVector(Value);
	}
};

/**
*	@brief Component attached to entities that are a child of another entity and/or that have children
*/
struct Hierarchy final
{
	entt::entity Parent{entt::null};
	entt::entity Previous{entt::null};
	entt::entity Next{entt::null};
	entt::entity FirstChild{entt::null};

	constexpr bool HasParent() { return Parent != entt::null; }

	constexpr bool HasSiblings() { return Previous != entt::null || Next != entt::null; }

	constexpr bool HasChildren() { return FirstChild != entt::null; }
};

inline entt::entity GetParent(entt::registry& registry, entt::entity entity)
{
	assert(registry.valid(entity));

	if (auto hierarchy = registry.try_get<Hierarchy>(entity); hierarchy)
	{
		return hierarchy->Parent;
	}

	return entt::null;
}

inline void SetParent(entt::registry& registry, entt::entity entity, entt::entity parent)
{
	assert(registry.valid(entity));

	if (entity == parent)
	{
		//TODO: log error
		return;
	}

	if (parent != entt::null)
	{
		//Verify that the given parent is not a child of this entity
		//Only the immediate parent has to be checked for null since the presence of a Hierarchy component implies a valid chain until Parent is null
		if (auto ancestorCheck = registry.try_get<Hierarchy>(parent); ancestorCheck)
		{
			for (;
				ancestorCheck->Parent != entt::null;
				ancestorCheck = &registry.get<Hierarchy>(ancestorCheck->Parent))
			{
				if (ancestorCheck->Parent == entity)
				{
					//TODO: log error
					return;
				}
			}
		}
	}

	//Check if the entity already has a parent
	auto hierarchy = registry.try_get<Hierarchy>(entity);

	if (hierarchy)
	{
		//Already a child of given parent, do nothing
		if (hierarchy->Parent == parent)
		{
			return;
		}

		//Patch up children chain
		if (hierarchy->Previous != entt::null)
		{
			auto& previousHierarchy = registry.get<Hierarchy>(hierarchy->Previous);

			previousHierarchy.Next = hierarchy->Next;
		}

		if (hierarchy->Next != entt::null)
		{
			auto& nextHierarchy = registry.get<Hierarchy>(hierarchy->Next);

			nextHierarchy.Previous = hierarchy->Previous;
		}

		{
			auto& parentHierarchy = registry.get<Hierarchy>(hierarchy->Parent);

			if (parentHierarchy.FirstChild == entity)
			{
				parentHierarchy.FirstChild = hierarchy->Next;
			}

			//Remove the hierarchy component if the parent no longer has any children and no parent of its own
			if (!parentHierarchy.HasChildren() && !parentHierarchy.HasParent())
			{
				registry.remove<Hierarchy>(hierarchy->Parent);
				registry.remove<LocalToParent>(hierarchy->Parent);
			}
		}

		hierarchy->Parent = entt::null;
		hierarchy->Previous = entt::null;
		hierarchy->Next = entt::null;
	}

	if (parent != entt::null)
	{
		//Add it to the parent
		if (!hierarchy)
		{
			hierarchy = &registry.assign<Hierarchy>(entity);
		}

		//Ensure that this exists
		registry.get_or_assign<LocalToParent>(entity);

		hierarchy->Parent = parent;

		auto& children = registry.get_or_assign<Hierarchy>(parent);

		if (children.FirstChild != entt::null)
		{
			auto& nextHierarchy = registry.get<Hierarchy>(children.FirstChild);

			nextHierarchy.Previous = entity;
			hierarchy->Next = children.FirstChild;
			hierarchy->Previous = entt::null;
		}

		children.FirstChild = entity;
	}
	else if (hierarchy)
	{
		registry.remove<LocalToParent>(entity);

		//Remove the component if it has no parent and no children
		if (!hierarchy->HasChildren())
		{
			registry.remove<Hierarchy>(entity);
		}
	}
}

inline void ClearParent(entt::registry& registry, entt::entity entity)
{
	SetParent(registry, entity, entt::null);
}

inline glm::vec3 CalculateAbsoluteRotationEulerXYZ(entt::registry& registry, entt::entity entity)
{
	assert(registry.valid(entity));

	const auto& rotationEulerXYZ = registry.get<RotationEulerXYZ>(entity);

	auto rotationInDegrees = rotationEulerXYZ.Value;

	for (auto parent = game::components::GetParent(registry, entity); parent != entt::null; parent = game::components::GetParent(registry, parent))
	{
		if (auto parentRotation = registry.try_get<RotationEulerXYZ>(parent); parentRotation)
		{
			rotationInDegrees += parentRotation->Value;
		}
	}

	return math::FixAngles(rotationInDegrees);
}
}
