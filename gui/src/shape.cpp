#include "gui/shape.hpp"
#include "gui/shader.hpp"

Shape::Shape(Kernel::Tree t)
    : tree(t), vert_vbo(QOpenGLBuffer::VertexBuffer),
      tri_vbo(QOpenGLBuffer::IndexBuffer)
{
    connect(&mesh_watcher, &decltype(mesh_watcher)::finished,
            this, &Shape::onFutureFinished);
}

void Shape::draw(const QMatrix4x4& M)
{
    if (mesh && !gl_ready)
    {
        initializeOpenGLFunctions();

        GLfloat* verts = new GLfloat[mesh->verts.size() * 6];
        unsigned i = 0;
        for (auto& v : mesh->verts)
        {
            verts[i++] = v.x();
            verts[i++] = v.y();
            verts[i++] = v.z();
            verts[i++] = 1;
            verts[i++] = 1;
            verts[i++] = 1;
        }
        vert_vbo.create();
        vert_vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
        vert_vbo.bind();
        vert_vbo.allocate(verts, i*sizeof(*verts));
        delete [] verts;

        uint32_t* tris = new uint32_t[mesh->tris.size() * 3];
        i = 0;
        for (auto& t: mesh->tris)
        {
            tris[i++] = t[0];
            tris[i++] = t[1];
            tris[i++] = t[2];
        }
        tri_vbo.create();
        tri_vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
        tri_vbo.bind();
        tri_vbo.allocate(tris, i*sizeof(*tris));
        delete [] tris;

        vao.create();
        vao.bind();
        vert_vbo.bind();
        tri_vbo.bind();
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), NULL);
        glVertexAttribPointer(
                1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat),
                (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        gl_ready = true;
    }

    if (gl_ready)
    {
        Shader::shaded->bind();
        glUniformMatrix4fv(Shader::shaded->uniformLocation("M"), 1, GL_FALSE, M.data());
        glUniform1f(Shader::shaded->uniformLocation("zoom"), 1);
        vao.bind();
        glDrawElements(GL_TRIANGLES, mesh->tris.size() * 3, GL_UNSIGNED_INT, NULL);
        vao.release();
        Shader::shaded->release();
    }
}

void Shape::startRender()
{
    assert(mesh.data() == nullptr);
    mesh_future = QtConcurrent::run(this, &Shape::renderMesh);
    mesh_watcher.setFuture(mesh_future);
}

Kernel::Mesh* Shape::renderMesh()
{
    Kernel::Region r({-1, 1}, {-1, 1}, {-1, 1}, 10);
    auto m = Kernel::Mesh::render(tree, r);
    return m.release();
}

void Shape::onFutureFinished()
{
    mesh.reset(mesh_future.result());
    gl_ready = false;
    emit(gotMesh());
}
