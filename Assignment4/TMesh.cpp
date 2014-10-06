#include "TMesh.h"
#include "FrameBuffer.h"
#include "scene.h"
#include <libtiff/tiffio.h>
#include <fstream>
#include <iostream>
#include <limits>

using namespace std;

TMesh::TMesh() {
	verts = NULL;
	vertsN = 0;
	tris = NULL;
	trisN = 0;

	texture = NULL;
	cubeMap = NULL;
	projector = NULL;
	raytracing = false;
}

TMesh::~TMesh() {
	if (texture != NULL) {
		delete texture;
	}
	if (cubeMap != NULL) {
		delete cubeMap;
	}
}

void TMesh::SetRaytracing() {
	raytracing = true;
}

/**
 * Load a mesh from bin file.
 */
void TMesh::Load(char* filename) {
	Clear();

	ifstream ifs(filename, ios::binary);
	if (ifs.fail()) {
		cerr << "INFO: cannot open file: " << filename << endl;
		return;
	}

	ifs.read((char*)&vertsN, sizeof(int));
	char v_yn, c_yn, n_yn, t_yn;
	ifs.read(&v_yn, 1); // always xyz
	if (v_yn != 'y') {
		cerr << "INTERNAL ERROR: there should always be vertex xyz data" << endl;
		return;
	}

	verts = new Vertex[vertsN];

	ifs.read(&c_yn, 1); // cols 3 floats
	ifs.read(&n_yn, 1); // normals 3 floats
	ifs.read(&t_yn, 1); // texture coordinates 2 floats

	for (int i = 0; i < vertsN; i++) {
		ifs.read((char*)&verts[i].v[0], 3 * sizeof(float));
	}

	if (c_yn == 'y') {
		for (int i = 0; i < vertsN; i++) {
			ifs.read((char*)&verts[i].c[0], 3 * sizeof(float));
		}
	}

	if (n_yn == 'y') {
		for (int i = 0; i < vertsN; i++) {
			ifs.read((char*)&verts[i].n[0], 3 * sizeof(float));
		}
	}

	if (t_yn == 'y') {
		for (int i = 0; i < vertsN; i++) {
			ifs.read((char*)&verts[i].t[0], 2 * sizeof(float));
		}
	}

	ifs.read((char*)&trisN, sizeof(int));
	tris = new unsigned int[trisN*3];
	ifs.read((char*)tris, trisN*3*sizeof(unsigned int)); // read tiangles

	ifs.close();

	//cerr << "INFO: loaded " << vertsN << " verts, " << trisN << " tris from " << endl << "      " << filename << endl;
	//cerr << "      xyz " << ((cols) ? "rgb " : "") << ((norms) ? "nxnynz " : "") << ((tcs) ? "tcstct " : "") << endl;
}

/**
 * Compute 3D axis aligned bouding box.
 *
 * @param p0	the corner with the minimum coordinates
 * @param p1	the corner with the maximum coordinates
 */
void TMesh::ComputeAABB(AABB &aabb) {
	for (int i = 0; i < vertsN; i++) {
		aabb.AddPoint(verts[i].v);
	}
}

/**
 * Translate all the vertices by the specified vector.
 *
 * @param v		the specified vector
 */
void TMesh::Translate(const V3 &v) {
	for (int i = 0; i < vertsN; i++) {
		verts[i].v += v;
	}
}

/**
 * Scale all the vertices by the specified factor.
 *
 * @param t		the specified scaling factor
 */
void TMesh::Scale(float t) {
	for (int i = 0; i < vertsN; i++) {
		verts[i].v *= t;
	}
}

/**
 * Scale this mesh by placing the centroid at givin position and scaling to given AABB size.
 *
 * @param centroid		the given position for the new centroid
 * @param size			the given AABB size
 */
void TMesh::Scale(const V3 &centroid, const V3 &size) {
	AABB aabb;
	ComputeAABB(aabb);

	V3 c = GetCentroid();
	
	V3 scale(size.x() / aabb.Size().x(), size.y() / aabb.Size().y(), size.z() / aabb.Size().z());

	for (int i = 0; i < vertsN; i++) {
		verts[i].v[0] = (verts[i].v.x() - c.x()) * scale.x() + centroid.x();
		verts[i].v[1] = (verts[i].v.y() - c.y()) * scale.y() + centroid.y();
		verts[i].v[2] = (verts[i].v.z() - c.z()) * scale.z() + centroid.z();
	}
}

void TMesh::RenderWireframe(FrameBuffer *fb, PPC *ppc) {
	for (int i = 0; i < trisN; i++) {
		fb->Draw3DSegment(ppc, verts[tris[i * 3]].v, verts[tris[i * 3]].c, verts[tris[i * 3 + 1]].v, verts[tris[i * 3 + 1]].c);
		fb->Draw3DSegment(ppc, verts[tris[i * 3 + 1]].v, verts[tris[i * 3 + 1]].c, verts[tris[i * 3 + 2]].v, verts[tris[i * 3 + 2]].c);
		fb->Draw3DSegment(ppc, verts[tris[i * 3 + 2]].v, verts[tris[i * 3 + 2]].c, verts[tris[i * 3]].v, verts[tris[i * 3]].c);
	}
}

void TMesh::Render(FrameBuffer *fb, PPC *ppc) {
	M33 camMat;
	camMat.SetColumn(0, ppc->a);
	camMat.SetColumn(1, ppc->b);
	camMat.SetColumn(2, ppc->c);

	// project all the vertices beforehand
	for (int i = 0; i < vertsN; i++) {
		ppc->Project(verts[i].v, verts[i].pv);
	}

	if (projector != NULL) {
		M33 camMat2;
		camMat2.SetColumn(0, projector->ppc->a);
		camMat2.SetColumn(1, projector->ppc->b);
		camMat2.SetColumn(2, projector->ppc->c);
		camMat2 = camMat2.Inverted();

		V3 Q1;
		Q1 = camMat2 * (ppc->C - projector->ppc->C);

		M33 Q2;
		Q2 = camMat2 * camMat;

		for (int i = 0; i < trisN; i++) {
			fb->rasterizeWithProjector(ppc, projector, camMat, Q1, Q2, verts[tris[i * 3]], verts[tris[i * 3 + 1]], verts[tris[i * 3 + 2]]);
		}
	} else if (raytracing) {
		for (int i = 0; i < trisN; i++) {
			fb->rasterizeByRayTracing(ppc, camMat, verts[tris[i * 3]], verts[tris[i * 3 + 1]], verts[tris[i * 3 + 2]], cubeMap, texture);
		}
	} else if (cubeMap != NULL) {
		cubeMap->SetCenter(GetCentroid());
		for (int i = 0; i < trisN; i++) {
			fb->rasterizeWithCubeMap(ppc, camMat, verts[tris[i * 3]], verts[tris[i * 3 + 1]], verts[tris[i * 3 + 2]], cubeMap);
		}
	} else if (texture != NULL) {
		for (int i = 0; i < trisN; i++) {
			fb->rasterizeWithTexture(ppc, camMat, verts[tris[i * 3]], verts[tris[i * 3 + 1]], verts[tris[i * 3 + 2]], texture);
		}
	} else {
		for (int i = 0; i < trisN; i++) {
			fb->rasterize(ppc, camMat, verts[tris[i * 3]], verts[tris[i * 3 + 1]], verts[tris[i * 3 + 2]]);
		}
	}
}

/**
 * Render the mesh by using the projective texture mapping.
 *
 * @param fb0		the reference image plane
 * @param ppc0		the reference camera
 * @param fb1		the desired image plane
 * @param ppc1		the desired camera
 */
void TMesh::RenderProjectiveTextureMapping(FrameBuffer *fb0, PPC *ppc0, FrameBuffer *fb1, PPC *ppc1) {
	M33 camMat0;
	camMat0.SetColumn(0, ppc0->a);
	camMat0.SetColumn(1, ppc0->b);
	camMat0.SetColumn(2, ppc0->c);
	camMat0 = camMat0.Inverted();

	M33 camMat1;
	camMat1.SetColumn(0, ppc1->a);
	camMat1.SetColumn(1, ppc1->b);
	camMat1.SetColumn(2, ppc1->c);

	V3 Q1;
	Q1 = camMat0 * (ppc1->C - ppc0->C);

	M33 Q2;
	Q2 = camMat0 * camMat1;

	// project all the vertices beforehand
	for (int i = 0; i < vertsN; i++) {
		ppc1->Project(verts[i].v, verts[i].pv);
	}

	for (int i = 0; i < trisN; i++) {
		fb1->RasterizeProjectiveTextureMapping(ppc1, ppc0, fb0, camMat1, Q1, Q2, verts[tris[i * 3]], verts[tris[i * 3 + 1]], verts[tris[i * 3 + 2]]);
	}
}

/**
 * Render the mesh by using the shadow mapping.
 *
 * @param fb0		the reference image plane
 * @param ppc0		the reference camera
 * @param fb1		the desired image plane
 * @param ppc1		the desired camera
 */
void TMesh::RenderShadowMapping(FrameBuffer *fb0, PPC *ppc0, FrameBuffer *fb1, PPC *ppc1) {
	M33 camMat0;
	camMat0.SetColumn(0, ppc0->a);
	camMat0.SetColumn(1, ppc0->b);
	camMat0.SetColumn(2, ppc0->c);
	camMat0 = camMat0.Inverted();

	M33 camMat1;
	camMat1.SetColumn(0, ppc1->a);
	camMat1.SetColumn(1, ppc1->b);
	camMat1.SetColumn(2, ppc1->c);

	V3 Q1;
	Q1 = camMat0 * (ppc1->C - ppc0->C);

	M33 Q2;
	Q2 = camMat0 * camMat1;

	// project all the vertices beforehand
	for (int i = 0; i < vertsN; i++) {
		ppc1->Project(verts[i].v, verts[i].pv);
	}

	for (int i = 0; i < trisN; i++) {
		fb1->RasterizeShadowMapping(ppc1, ppc0, fb0, camMat1, Q1, Q2, verts[tris[i * 3]], verts[tris[i * 3 + 1]], verts[tris[i * 3 + 2]]);
	}
}

/**
 * Clear the allocated memory for vertices.
 */
void TMesh::Clear() {
	if (verts != NULL) {
		delete [] verts;
	}
	verts = NULL;
	
	vertsN = 0;

	if (tris != NULL) {
		delete [] tris;
	}
	trisN = 0;
}

/**
 * Rotate this mesh around the specified axis passing through the centroid by the specified angle.
 *
 * @param axis		the specified axis
 * @param angle		the specified angle
 */
void TMesh::RotateAbout(const V3 &axis, float angle) {
	RotateAbout(axis, angle, GetCentroid());
}

/**
 * Rotate this mesh around the specified axis passing through the specified origin by the specified angle.
 *
 * @param axis		the specified axis
 * @param angle		the specified angle
 * @param orig		the specified origin
 */
void TMesh::RotateAbout(const V3 &axis, float angle, const V3 &orig) {
	for (int i = 0; i < vertsN; i++) {
		verts[i].v = verts[i].v.RotateAbout(axis, angle, orig);
	}
}

/**
 * Return the center of axis aligned bounding box of vertices.
 *
 * @return		the center of axis aligned bounding box
 */
V3 TMesh::GetCentroid() {
	AABB aabb;
	ComputeAABB(aabb);

	return (aabb.maxCorner() + aabb.minCorner()) / 2.0f;
}

bool TMesh::isInside2D(const V3 &p0, const V3 &p1, const V3 &p2, const V3 p) const {
	float den = ((p1 - p0) ^ (p2 - p0)).z();
	float s = ((p - p0) ^ (p2 - p0)).z() / den;
	float t = ((p - p0) ^ (p1 - p0)).z() / den;

	if (s < 0 || s > 1) return false;
	if (t < 0 || t > 1) return false;
	if (s + t > 1) return false;
	return true;
}

void TMesh::SetTexture(const char* filename) {
	texture = new Texture(filename);
}

void TMesh::SetCubeMap(const TIFFImage &image) {
	cubeMap = new CubeMap(image);
}

bool TMesh::RayTrace(PPC* ppc, const V3 &p, const V3 &dir, V3 &col, float &dist) {
	float dydx = dir.y() / dir.x();
	float dzdx = dir.z() / dir.x();

	for (int i = 0; i < trisN; i++) {
		Vertex p0 = verts[tris[i * 3]];
		Vertex p1 = verts[tris[i * 3 + 1]];
		Vertex p2 = verts[tris[i * 3 + 2]];

		float a = -dydx * p0.v.x() + dydx * p1.v.x() + p0.v.y() - p1.v.y();
		float b = dydx * p2.v.x() - dydx * p0.v.x() + p0.v.y() - p2.v.y();
		float c = -p.y() - dydx * p0.v.x() + dydx * p.x() + p0.v.y();
		float d = -dzdx * p0.v.x() + dzdx * p1.v.x() + p0.v.z() - p1.v.z();
		float e = -dzdx * p0.v.x() + dzdx * p2.v.x() + p0.v.z() - p2.v.z();
		float f = -p.z() - dzdx * p0.v.x() + dzdx * p.x() + p0.v.z();

		float s = (c * e - b * f) / (a * e - b * d);
		float t = (c * d - a * f) / (b * d - a * e);
		if (s < 0 || s > 1 || t < 0 || t > 1 || s + t > 1) continue;

		dist = ((1.0f - s - t) * p0.v.x() + p1.v.x() * s + p2.v.x() * t - p.x()) / dir.x();
		if (dist <= 0.01f) continue;

		if (cubeMap != NULL) {
			col = FrameBuffer::GetColorFromCubeMap(ppc, p, p0, p1, p2, cubeMap, s, t);
			return true;
		} else if (texture != NULL) {
			col = FrameBuffer::GetColorFromTexture(ppc, p, p0, p1, p2, texture, s, t);
			return true;
		} else {
			col = FrameBuffer::GetColor(ppc, p, p0, p1, p2, s, t);
			//col = V3(0, 0, 1);
			return true;
		}
	}

	return false;
}

void TMesh::SetProjector(Projector* projector) {
	this->projector = projector;
}