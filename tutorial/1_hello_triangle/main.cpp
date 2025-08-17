#include <FCT.h>
using namespace FCT;
int main()
{
    Runtime rt;
    auto wnd = rt.createWindow(800,600,"Hello, FCT!");
    auto ctx = rt.createContext();
    ctx->create();
    wnd->bind(ctx);

    auto graph = ctx->getModule<RenderGraph>();
    graph->addPass(
        "base",
        Target("WindowColor",wnd),
        EnablePassClear(ClearType::color)
    );
    graph->compile();

    VertexLayout vertexLayout {
        VertexElement{VtxType::Color4f},
        VertexElement{VtxType::Position2f}
    };
    Layout layout {
        ctx,
        vertexLayout,
        PassName("base") //Render Graph必须已经执行了 compile
    };

    DynamicMesh<uint32_t> mesh(ctx,vertexLayout);
    mesh.addVertex(
        Vec4(1,0,0,1), //对应第一个顶点属性, 颜色
        Vec2(0.0f,-0.5f) //对应第二个顶点属性， 坐标 坐标系采用的是vulkan的坐标系
    );
    mesh.addVertex(
        Vec4(0,1,0,1),
        Vec2(-0.5f,0.5f)
    );
    mesh.addVertex(
        Vec4(0,0,1,1),
        Vec2(0.5f,0.5f)
    );
    mesh.create();

    graph->subscribe("base",[&layout,&mesh,wnd](PassSubmitEvent env)
    {
        auto cmdBuf = env.cmdBuf;

        /**
            pass开始时必须执行 cmdBuf的下面这俩个函数，否则会是一片黑
                virtual void viewport(Vec2 lt,Vec2 rb) = 0;
                virtual void scissor(Vec2 lt,Vec2 rb) = 0;
            使用wnd->getModule<WindowModule::AutoViewport>(); 获取autoviewport 模块
            viewport->submit(cmdBuf);内部会调用viewport和scissor，使得渲染出来的画面
            保持创建窗口时的比例
        **/

        auto viewport = wnd->getModule<WindowModule::AutoViewport>();
        viewport->submit(cmdBuf);


        layout.begin();
        layout.drawMesh(cmdBuf,mesh);
        layout.end();
    });

    while (wnd->isRunning())
    {
        ctx->flush();
    }

    return 0;
}