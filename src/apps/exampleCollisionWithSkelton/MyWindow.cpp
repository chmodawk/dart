#include "MyWindow.h"
#include "dynamics/SkeletonDynamics.h"
#include "utils/UtilsMath.h"
#include "utils/Timer.h"
#include "yui/GLFuncs.h"
#include <cstdio>
#include "yui/GLFuncs.h"
#include "kinematics/BodyNode.h"
#include "kinematics/Primitive.h"

using namespace Eigen;
using namespace kinematics;
using namespace utils;
using namespace integration;

void evalRT(MatrixXd mat, Vec3f R[3], Vec3f& T)
{
	//mat.transposeInPlace();
	for(int i=0;i<3;i++)
		for(int j=0;j<3;j++)
			R[i][j] = mat(j, i);
	for(int i=0;i<3;i++)
		T[i] = mat(i, 3);
}

template<class BV>
void drawBVHModel(BVHModel<BV>& model)
{

	glBegin(GL_TRIANGLES);
	for(int i=0;i<model.num_tris;i++)
	{
		Triangle tri = model.tri_indices[i];
		glVertex3f(model.vertices[tri[0]][0], model.vertices[tri[0]][1], model.vertices[tri[0]][2]);
		glVertex3f(model.vertices[tri[1]][0], model.vertices[tri[1]][1], model.vertices[tri[1]][2]);
		glVertex3f(model.vertices[tri[2]][0], model.vertices[tri[2]][1], model.vertices[tri[2]][2]);
	}
	glEnd();

}

void MyWindow::initDyn()
{
    // set random initial conditions
    mDofs.resize(mModel->getNumDofs());
    mDofVels.resize(mModel->getNumDofs());
    for(unsigned int i=0; i<mModel->getNumDofs(); i++){
        mDofs[i] = utils::random(-0.5,0.5);
        mDofVels[i] = utils::random(-0.1,0.1);
    }
    mModel->setPose(mDofs,false,false);

	//init collision mesh
    mContactCheck.addCollisionSkeletonNode(mModel->getRoot(), true);
    mContactCheck.addCollisionSkeletonNode(mModel2->getRoot(), true);
    
    //mContactCheck.addCollisionMesh(createCube<RSS>(0.05, 0.05, 0.05));
    //mContactCheck.addCollisionMesh(createCube<RSS>(0.05, 0.05, 0.05));
    printf("create model ok");
}

VectorXd MyWindow::getState() {
    VectorXd state(mDofs.size() + mDofVels.size());
    state.head(mDofs.size()) = mDofs;
    state.tail(mDofVels.size()) = mDofVels;
    return state;
}

VectorXd MyWindow::evalDeriv() {
    VectorXd deriv(mDofs.size() + mDofVels.size());
    VectorXd qddot = -mModel->getMassMatrix().fullPivHouseholderQr().solve( mModel->getCombinedVector() ); 
    mModel->clampRotation(mDofs, mDofVels);
    deriv.tail(mDofVels.size()) = qddot; // set qddot (accelerations)
    deriv.head(mDofs.size()) = (mDofVels + (qddot * mTimeStep)); // set velocities
    return deriv;
}

void MyWindow::setState(VectorXd newState) {
    mDofVels = newState.tail(mDofVels.size());
    mDofs = newState.head(mDofs.size());
}

void MyWindow::setPose() {
    mModel->setPose(mDofs,false,false);
    mModel->computeDynamics(mGravity, mDofVels, true);
}

void MyWindow::displayTimer(int _val)
{
    static Timer tSim("Simulation");
    int numIter = mDisplayTimeout / (mTimeStep*1000);
    for(int i=0; i<numIter; i++){
        tSim.startTimer();
        setPose();
        mIntegrator.integrate(this, mTimeStep);
        tSim.stopTimer();
    }

    mContactCheck.checkCollision();
    printf("detect %d contacts", mContactCheck.getNumContact());


    mFrame += numIter;   
    glutPostRedisplay();
    if(mRunning)	
        glutTimerFunc(mDisplayTimeout, refreshTimer, _val);
}

void MyWindow::draw()
{
    glDisable(GL_LIGHTING);
    //mModel->draw(mRI);
    //if(mShowMarker) mModel->drawMarkers(mRI);

    //collision and draw
    MatrixXd worldTrans(4, 4);

   

    bool bCollide = false;
    MatrixXd matCOM;
    matCOM.setIdentity(4, 4);

    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//     for(int i=0;i<mContactCheck.mCollisionSkeltonNodeList.size();i++)
//     {
//         printf("%d\n",i);
//         worldTrans.setIdentity(4, 4);
//         matCOM.setIdentity(4, 4);
//         if(mContactCheck.mCollisionSkeltonNodeList[i]->bodyNode !=NULL)
//         {
//             for(int j=0;j<3;j++)
//             {
//                 matCOM(j, 3) = mContactCheck.mCollisionSkeltonNodeList[i]->bodyNode->getLocalCOM()[j];
//             }
//             worldTrans = mContactCheck.mCollisionSkeltonNodeList[i]->bodyNode->getWorldTransform()*matCOM;
//         }
// 
// 
// 
//         glPushMatrix();
//         glMultMatrixd(worldTrans.data());
// 
//         drawBVHModel(*mContactCheck.mCollisionSkeltonNodeList[i]->cdmesh);
//         glPopMatrix();
//     }

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    mModel->draw(mRI);
    mModel2->draw(mRI);



     

    glBegin(GL_LINES);
    
    for(int k=0;k<mContactCheck.getNumContact();k++)
    {
        Eigen::Vector3d  v = mContactCheck.getContact(k).point;
        Eigen::Vector3d n = mContactCheck.getContact(k).normal;
        glVertex3f(v[0], v[1], v[2]);
       /* printf("%f %f %f\n", v[0], v[1], v[2]);*/
        glVertex3f(v[0]+n[0], v[1]+n[1], v[2]+n[2]);
    }
    glEnd();

    //drawBVHModel(*mBody[0]);
//     glPushMatrix();
//     glTranslatef(0, -0.4, 0);
//     glColor3f(0.9, 0.9, 0.9);
//     //glutSolidCube(0.05);
//     glPopMatrix();
    
    // display the frame count in 2D text
    char buff[64];
    sprintf(buff,"%d",mFrame);
    string frame(buff);
    glDisable(GL_LIGHTING);
    glColor3f(0.0,0.0,0.0);
    yui::drawStringOnScreen(0.02f,0.02f,frame);
    glEnable(GL_LIGHTING);
}

void MyWindow::keyboard(unsigned char key, int x, int y)
{
    switch(key){
    case ' ': // use space key to play or stop the motion
        mRunning = !mRunning;
        if(mRunning)
            glutTimerFunc( mDisplayTimeout, refreshTimer, 0);
        break;
    case 'r': // reset the motion to the first frame
        mFrame = 0;
        break;
    case 'h': // show or hide markers
        mShowMarker = !mShowMarker;
        break;
    default:
        Win3D::keyboard(key,x,y);
    }
    glutPostRedisplay();
}


