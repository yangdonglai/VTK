// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This example tests the vtkRenderingExternal module by drawing a GLUT window
// and rendering a VTK cube in it. It uses an ExternalVTKWidget and sets a
// vtkExternalOpenGLRenderWindow to it.
//
// The test also demonstrates the use of
// PreserveColorBuffer and PreserveDepthBuffer flags on the
// vtkExternalOpenGLRenderer by drawing a GL_TRIANGLE in the scene before
// drawing the vtk sphere.

#include <vtk_glew.h>
// GLUT includes
#if defined(__APPLE__)
#include <AvailabilityMacros.h>
#if MAC_OS_X_VERSION_MIN_REQUIRED > 1090
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#include <GLUT/glut.h> // Include GLUT API.
#else
#if defined(_WIN32)
#include "vtkWindows.h" // Needed to include OpenGL header on Windows.
#endif                  // _WIN32
#include <GL/glut.h>    // Include GLUT API.
#endif

// STD includes
#include <iostream>

// VTK includes
#include <ExternalVTKWidget.h>
#include <vtkActor.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCubeSource.h>
#include <vtkExternalOpenGLRenderWindow.h>
#include <vtkLight.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkTesting.h>

namespace
{

// Global variables used by the glutDisplayFunc and glutIdleFunc
ExternalVTKWidget* externalVTKWidget = nullptr;
bool initialized = false;
int NumArgs;
char** ArgV;
bool tested = false;
int retVal = 0;
int windowId = -1;
int windowH = 301;
int windowW = 300;

void MakeCurrentCallback(vtkObject* vtkNotUsed(caller), long unsigned int vtkNotUsed(eventId),
  void* vtkNotUsed(clientData), void* vtkNotUsed(callData))
{
  vtkLogScopeFunction(1);
  if (initialized)
  {
    glutSetWindow(windowId);
  }
}

/* Handler for window-repaint event. Call back when the window first appears and
   whenever the window needs to be re-painted. */
void display()
{
  vtkLogScopeFunction(INFO);
  if (!initialized)
  {
    vtkLogScopeF(INFO, "do-initialize");
    // since `handleResize` may get called before display, we may have already
    // created and resized the vtkExternalOpenGLRenderWindow, hence we don't
    // recreate it here.
    auto renWin = externalVTKWidget->GetRenderWindow();

    // since our example here is not setting up the `glViewport`, we don't want
    // the vtkExternalOpenGLRenderWindow to update its size based on the
    // glViewport hence we must disable automatic position and size.
    renWin->AutomaticWindowPositionAndResizeOff();

    assert(renWin != nullptr);
    vtkNew<vtkCallbackCommand> callback;
    callback->SetCallback(MakeCurrentCallback);
    renWin->AddObserver(vtkCommand::WindowMakeCurrentEvent, callback);
    vtkNew<vtkPolyDataMapper> mapper;
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    vtkRenderer* ren = externalVTKWidget->AddRenderer();
    ren->AddActor(actor);
    vtkNew<vtkCubeSource> cs;
    mapper->SetInputConnection(cs->GetOutputPort());
    actor->RotateX(45.0);
    actor->RotateY(45.0);
    ren->ResetCamera();

    initialized = true;
  }

  // Enable depth testing. Demonstrates OpenGL context being managed by external
  // application i.e. GLUT in this case.
  glEnable(GL_DEPTH_TEST);

  // Buffers being managed by external application i.e. GLUT in this case.
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set background color to black and opaque
  glClearDepth(1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the color buffer

  glFlush(); // Render now
  glBegin(GL_TRIANGLES);
  glVertex3f(-1.5, -1.5, 0.0);
  glVertex3f(1.5, 0.0, 0.0);
  glVertex3f(0.0, 1.5, 1.0);
  glEnd();

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  GLfloat lightpos[] = { -0.5f, 1.0f, 1.0f, 1.0f };
  glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
  GLfloat diffuse[] = { 0.0f, 0.8f, 0.8f, 1.0f };
  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
  GLfloat specular[] = { 0.5f, 0.0f, 0.0f, 1.0f };
  glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
  GLfloat ambient[] = { 1.0f, 1.0f, 0.2f, 1.0f };
  glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);

  vtkLogScopeF(INFO, "do-vtk-render");
  externalVTKWidget->GetRenderWindow()->Render();
  glutSwapBuffers();
}

void test()
{
  vtkLogScopeFunction(INFO);
  bool interactiveMode = false;
  vtkTesting* t = vtkTesting::New();
  for (int cc = 1; cc < NumArgs; cc++)
  {
    t->AddArgument(ArgV[cc]);
    if (strcmp(ArgV[cc], "-I") == 0)
    {
      interactiveMode = true;
    }
  }
  t->SetRenderWindow(externalVTKWidget->GetRenderWindow());
  if (!tested)
  {
    retVal = t->RegressionTest(0);
    tested = true;
  }
  t->Delete();
  if (!interactiveMode)
  {
    // Exit out of the infinitely running loop
    exit(!retVal);
  }
}

void handleResize(int w, int h)
{
  vtkLogScopeF(INFO, "handleResize: %d, %d", w, h);
  externalVTKWidget->GetRenderWindow()->SetSize(w, h);
  glutPostRedisplay();
}

void onexit()
{
  initialized = false;
}

} // end anon namespace

/* Main function: GLUT runs as a console application starting at main()  */
int TestGLUTRenderWindow(int argc, char* argv[])
{
  vtkNew<ExternalVTKWidget> staticExternalVTKWidget;
  externalVTKWidget = staticExternalVTKWidget;
  NumArgs = argc;
  ArgV = argv;
  glutInit(&argc, argv); // Initialize GLUT
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
  vtkLog(INFO, "glutInitWindowSize: " << windowW << ", " << windowH);
  glutInitWindowSize(windowW, windowH); // Set the window's initial width & height
  glutInitWindowPosition(101, 201);     // Position the window's initial top-left corner
  windowId = glutCreateWindow("VTK External Window Test"); // Create a window with the given title
  glutDisplayFunc(display);      // Register display callback handler for window re-paint
  glutIdleFunc(test);            // Register test callback handler for vtkTesting
  glutReshapeFunc(handleResize); // Register resize callback handler for window resize
  atexit(onexit);                // Register callback to uninitialize on exit
  glewInit();
  glutMainLoop(); // Enter the infinitely event-processing loop
  return 0;
}
