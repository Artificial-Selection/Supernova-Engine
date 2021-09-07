#pragma once


namespace snv
{

class Application;


class IApplicationLayer
{
    friend Application;

    virtual void OnCreate() = 0;
    virtual void OnDestroy() = 0;
    virtual void OnUpdate() = 0;
};

} // namespace snv
