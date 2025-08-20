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
        VertexElement{VtxType::Position4f}
    };
    Layout layout {
        ctx,
        vertexLayout,
        PassName("base"), //Render Graph必须已经执行了 compile
        UniformSlot(
            "uniform0",//这个名字基本上在着色器内部没有 作用，
            UniformVar(UniformType::Float,"time"),
            UniformVar(UniformType::Mat4,"mvp")
        )
    };
    auto vertexShader = layout.allocateVertexShader(R"(
ShaderOut main(ShaderIn sIn) {
    ShaderOut sOut;
    float brightness = 1.0f + 0.5f * sin(time * 2.0f);
    sOut.color = sIn.color * brightness;
    sOut.pos = mvp * sIn.pos; //我们使用列向量 所以是矩阵左乘向量
    return sOut;
}
)");
    auto uniform0 = layout.allocateUniform("uniform0");

    DynamicMesh<uint32_t> mesh(ctx,vertexLayout);

    // 添加4个顶点组成矩形
    mesh.addVertex(
        Vec4(1,0,0,1), // 红色 - 左下角
        Vec4(-0.5f,-0.5f,0.0f,1.0f)
    );
    mesh.addVertex(
        Vec4(0,1,0,1), // 绿色 - 右下角
        Vec4(0.5f,-0.5f,0.0f,1.0f)
    );
    mesh.addVertex(
        Vec4(0,0,1,1), // 蓝色 - 右上角
        Vec4(0.5f,0.5f,0.0f,1.0f)
    );
    mesh.addVertex(
        Vec4(1,1,0,1), // 黄色 - 左上角
        Vec4(-0.5f,0.5f,0.0f,1.0f)
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
        layout.bindUniform(uniform0);
        layout.bindVertexShader(vertexShader);
        layout.drawMesh(cmdBuf,mesh);
        layout.end();
    });
    auto startTime = std::chrono::high_resolution_clock::now();

    while (wnd->isRunning())
    {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float elapsed = std::chrono::duration<float>(currentTime - startTime).count();
        // 创建透视投影矩阵
        float fov = 90; // 90度视野角
        float aspect = 800.0f / 600.0f; // 窗口宽高比
        float nearPlane = 0.1f;
        float farPlane = 100.0f;

        Mat4 projection = Mat4::Perspective(
            fov,
            aspect,
            nearPlane,
            farPlane);
        Mat4 view = Mat4::LookAt(
            Vec3(3, 3, 3), // 摄像机位置
            Vec3(0, 0, 0), // 看向原点
            Vec3(0, 0, 1)  // 上方向
        );

        // 创建模型矩阵（旋转）
        Mat4 model;
        model.rotateZ(elapsed * 22.5f); // 绕Z轴旋转

        Mat4 mvp = projection * view * model;

        uniform0.setValue("time", elapsed);
        uniform0.setValue("mvp", mvp);
        uniform0.update();
        ctx->flush();
    }

    return 0;
}