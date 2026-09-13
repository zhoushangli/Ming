#include "MingEngine/Core/Object/PlaceHolderScriptInstance.hpp"

PlaceHolderScriptInstance::~PlaceHolderScriptInstance() 
{
    m_owner = nullptr;
    m_script = nullptr;
}
