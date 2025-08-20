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
    auto vertexShader = layout.allocateVertexShader(R"(
ShaderOut main(ShaderIn sIn) {
    ShaderOut sOut;
    sOut.color = sIn.color * 1.1f; //试着改一下这个1.1
    sOut.pos = sIn.pos;
    return sOut;
}
)");

    DynamicMesh<uint32_t> mesh(ctx,vertexLayout);

    // 添加4个顶点组成矩形
    mesh.addVertex(
        Vec4(1,0,0,1), // 红色 - 左下角
        Vec2(-0.5f,-0.5f)
    );
    mesh.addVertex(
        Vec4(0,1,0,1), // 绿色 - 右下角
        Vec2(0.5f,-0.5f)
    );
    mesh.addVertex(
        Vec4(0,0,1,1), // 蓝色 - 右上角
        Vec2(0.5f,0.5f)
    );
    mesh.addVertex(
        Vec4(1,1,0,1), // 黄色 - 左上角
        Vec2(-0.5f,0.5f)
    );

    // 添加索引组成两个三角形
    // 第一个三角形: 左下 -> 右下 -> 右上
    mesh.addIndex(0);
    mesh.addIndex(1);
    mesh.addIndex(2);

    // 第二个三角形: 左下 -> 右上 -> 左上
    mesh.addIndex(0);
    mesh.addIndex(2);
    mesh.addIndex(3);

    mesh.create();

    graph->subscribe("base",[&](PassSubmitEvent env)
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
        layout.bindVertexShader(vertexShader);
        layout.drawMesh(cmdBuf,mesh);
        layout.end();
    });

    while (wnd->isRunning())
    {
        ctx->flush();
    }

    return 0;
}