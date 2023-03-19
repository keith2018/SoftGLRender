/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include <iostream>
#include <sstream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Base/Logger.h"
#include "Render/OpenGL/GLSLUtils.h"
#include "Viewer/ViewerManager.h"

std::shared_ptr<SoftGL::View::ViewerManager> viewer = nullptr;

const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 800;

double lastX = SCR_WIDTH / 2.0f;
double lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

void framebufferSizeCallback(GLFWwindow *window, int width, int height);
void mouseCallback(GLFWwindow *window, double xPos, double yPos);
void scrollCallback(GLFWwindow *window, double xOffset, double yOffset);
void processInput(GLFWwindow *window);

const char *VS = R"(
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
	gl_Position = vec4(aPos, 1.0);
	TexCoord = vec2(aTexCoord.x, aTexCoord.y);
}
)";

const char *FS = R"(
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;

void main()
{
    FragColor = texture(uTexture, TexCoord);
}
)";

static void glfwErrorCallback(int error, const char *description) {
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main() {
  /* Initialize the library */
  glfwSetErrorCallback(glfwErrorCallback);
  if (!glfwInit()) {
    LOGE("Failed to initialize GLFW");
    return -1;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  /* create a windowed mode window and its OpenGL context */
  GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "SoftGLRenderer", nullptr, nullptr);
  if (!window) {
    LOGE("Failed to create GLFW window");
    glfwTerminate();
    return -1;
  }

  /* Make the window's context current */
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
  glfwSetCursorPosCallback(window, mouseCallback);
  glfwSetScrollCallback(window, scrollCallback);

  // tell GLFW to capture our mouse
//    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  /* Load all OpenGL function pointers */
  if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
    LOGE("Failed to initialize GLAD");
    glfwTerminate();
    return -1;
  }

  SoftGL::ProgramGLSL program;
  if (!program.loadSource(VS, FS)) {
    LOGE("Failed to initialize Shader");
    glfwTerminate();
    return -1;
  }

  // set up vertex data (and buffer(s)) and configure vertex attributes
  float vertices[] = {
      // positions | texture coords
      1.f, 1.f, 0.0f, 1.0f, 1.0f, // top right
      1.f, -1.f, 0.0f, 1.0f, 0.0f, // bottom right
      -1.f, -1.f, 0.0f, 0.0f, 0.0f, // bottom left
      -1.f, 1.f, 0.0f, 0.0f, 1.0f  // top left
  };
  unsigned int indices[] = {
      0, 1, 3, // first triangle
      1, 2, 3  // second triangle
  };
  GLuint VAO, VBO, EBO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glGenBuffers(1, &EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  glBindVertexArray(GL_NONE);

  // load and create a texture
  // -------------------------
  unsigned int texture;
  glGenTextures(1, &texture);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  // set the texture wrapping parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // set texture filtering parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  program.use();
  glUniform1i(glGetUniformLocation(program.getId(), "uTexture"), 0);

  // init Viewer
  viewer = std::make_shared<SoftGL::View::ViewerManager>();
  bool initSuccess = viewer->create(window, SCR_WIDTH, SCR_HEIGHT, (int) texture);
  if (!initSuccess) {
    LOGE("Failed to create Viewer");
  } else {
    // real frame buffer size
    int frameWidth, frameHeight;
    glfwGetFramebufferSize(window, &frameWidth, &frameHeight);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)) {
      // check exit app
      processInput(window);

      // draw frame
      viewer->drawFrame();

      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glViewport(0, 0, frameWidth, frameHeight);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

      glDisable(GL_BLEND);
      glDisable(GL_DEPTH_TEST);
      glDisable(GL_CULL_FACE);
      glDepthMask(true);

      glClearColor(0.f, 0.f, 0.f, 0.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture);

      program.use();
      glBindVertexArray(VAO);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

      viewer->drawPanel();
      glfwSwapBuffers(window);
      glfwPollEvents();
    }
  }

  viewer->destroy();
  viewer = nullptr;

  program.destroy();

  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);
  glDeleteTextures(1, &texture);

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
  if (!viewer || viewer->wantCaptureKeyboard()) {
    return;
  }

  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
    return;
  }

  static bool keyPressed_H = false;
  int state = glfwGetKey(window, GLFW_KEY_H);
  if (state == GLFW_PRESS) {
    if (!keyPressed_H) {
      keyPressed_H = true;
      viewer->togglePanelState();
    }
  } else if (state == GLFW_RELEASE) {
    keyPressed_H = false;
  }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebufferSizeCallback(GLFWwindow *window, int width, int height) {
  // make sure the viewport matches the new window dimensions; note that width and
  // height will be significantly larger than specified on retina displays.
  glViewport(0, 0, width, height);

  if (!viewer) {
    return;
  }
  viewer->updateSize(width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouseCallback(GLFWwindow *window, double xPos, double yPos) {
  if (!viewer || viewer->wantCaptureMouse()) {
    return;
  }

  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
    if (firstMouse) {
      lastX = xPos;
      lastY = yPos;
      firstMouse = false;
    }

    double xOffset = xPos - lastX;
    double yOffset = yPos - lastY;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      viewer->updateGesturePan((float) xOffset, (float) yOffset);
    } else {
      viewer->updateGestureRotate((float) xOffset, (float) yOffset);
    }

    lastX = xPos;
    lastY = yPos;
  } else {
    firstMouse = true;
  }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scrollCallback(GLFWwindow *window, double xOffset, double yOffset) {
  if (!viewer || viewer->wantCaptureMouse()) {
    return;
  }

  viewer->updateGestureZoom((float) xOffset, (float) yOffset);
}
