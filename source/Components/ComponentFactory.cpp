//
// Created by Devilast on 6/27/2021.
//

#include <Components/ComponentFactory.hpp>


namespace snv
{

entt::entity ComponentFactory::CreateEntity()
{
    return m_registry.create();
}

} // namespace snv
