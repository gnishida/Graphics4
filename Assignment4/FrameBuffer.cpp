﻿#include "FrameBuffer.h"
#include <libtiff/tiffio.h>
#include <iostream>
#include "scene.h"
#include <math.h>
#include <algorithm>

using namespace std;

FrameBuffer::FrameBuffer(int _w, int _h) {
	w = _w;
	h = _h;
	pix = new unsigned int[w*h];
	zb  = new float[w*h];
}

FrameBuffer::~FrameBuffer() {
	delete [] pix;
}

/**
 * Set all pixels to given color.
 *
 * @param bgr	the given color
 */
void FrameBuffer::Set(unsigned int bgr) {
	for (int uv = 0; uv < w*h; uv++) {
		pix[uv] = bgr;
	}
}

/**
 * Set one pixel to given color.
 * This function does not check neigher the range and the zbuffer.
 *
 * @param u		x coordinate of the pixel
 * @param v		y coordinate of the pixel
 * @param clr	the color
 */
void FrameBuffer::Set(int u, int v, unsigned int clr) {
	pix[(h-1-v)*w+u] = clr;
}

/**
 * Set one pixel to given color.
 * This function does not check the range, but check the zbuffer.
 *
 * @param u		x coordinate of the pixel
 * @param v		y coordinate of the pixel
 * @param clr	the color
 * @param z		z buffer
 */
void FrameBuffer::Set(int u, int v, unsigned int clr, float z) {
	if (zb[(h-1-v)*w+u] > z) return;

	pix[(h-1-v)*w+u] = clr;
	zb[(h-1-v)*w+u] = z;
}

/**
 * Set one pixel to given color.
 * If the specified pixel is out of the screen, it does nothing.
 *
 * @param u		x coordinate of the pixel
 * @param v		y coordinate of the pixel
 * @param clr	the color
 */
void FrameBuffer::SetGuarded(int u, int v, unsigned int clr, float z) {
	if (u < 0 || u > w-1 || v < 0 || v > h-1) return;

	Set(u, v, clr, z);
}

// set all z values in SW ZB to z0
void FrameBuffer::SetZB(float z0) {
	for (int i = 0; i < w*h; i++) {
		zb[i] = z0;
	}
}

/**
 * Draw 2D segment with color interpolation.
 *
 * @param p0	the first point of the segment
 * @param c0	the color of the first point
 * @param p1	the second point of the segment
 * @param c1	the color of the second point
 */
void FrameBuffer::Draw2DSegment(const V3 &p0, const V3 &c0, const V3 &p1, const V3 &c1) {
	float dx = fabsf(p0.x() - p1.x());
	float dy = fabsf(p0.y() - p1.y());

	int n;
	if (dx < dy) {
		n = 1 + (int)dy;
	} else {
		n = 1 + (int)dx;
	}

	for (int i = 0; i <= n; i++) {
		float frac = (float) i / (float)n;
		V3 curr = p0 + (p1-p0) * frac;
		V3 currc = c0 + (c1-c0) * frac;
		int u = (int)curr[0];
		int v = (int)curr[1];
		SetGuarded(u, v, currc.GetColor(), curr[2]);
	}
}

/**
 * Draw 3D segment with color interpolation.
 *
 * @param ppc	the camera
 * @param p0	the first point of the segment
 * @param c0	the color of the first point
 * @param p1	the second point of the segment
 * @param c1	the color of the second point
 */
void FrameBuffer::Draw3DSegment(PPC* ppc, const V3 &p0, const V3 &c0, const V3 &p1, const V3 &c1) {
	V3 pp0, pp1;
	if (!ppc->Project(p0, pp0)) return;
	if (!ppc->Project(p1, pp1)) return;

	Draw2DSegment(pp0, c0, pp1, c1);
}

/**
 * Draw axis aligned rectangle.
 *
 * @param p0	the top left corner of the rectangle
 * @param p1	the bottom right corner of the rectangle
 * @param c		the color
 */
void FrameBuffer::DrawRectangle(const V3 &p0, const V3 &p1, const V3 &c) {
	V3 p2(p0.x(), p1.y(), 0.0f);
	V3 p3(p1.x(), p0.y(), 0.0f);

	Draw2DSegment(p0, c, p2, c);
	Draw2DSegment(p2, c, p1, c);
	Draw2DSegment(p1, c, p3, c);
	Draw2DSegment(p3, c, p0, c);
}

/**
 * Load the frame buffer from the specified tiff file.
 *
 * @param filename		the tiff file name
 * @return				true if the load successes; false otherwise
 */
bool FrameBuffer::Load(char* filename) {
	TIFF* tiff = TIFFOpen(filename, "r");
	if (tiff == NULL) return false;

	int w2, h2;
	TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &w2);
	TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &h2);

	pix = (unsigned int*)_TIFFmalloc(sizeof(unsigned int) * w2 * h2);
	if (!TIFFReadRGBAImage(tiff, w, h, pix, 0)) {
		delete [] pix;
		pix = new unsigned int[w*h];
		TIFFClose(tiff);
		return false;
	}

	w = w2;
	h = h2;
	TIFFClose(tiff);

	return true;
}

/**
 * Save the frame buffer to a tiff file.
 *
 * @param filename		the file name to store the frame buffer
 * @return				true if the save successes; false otherwise
 */
bool FrameBuffer::Save(char* filename) {
	unsigned int *temp = new unsigned int[w * h];
	for (int v = 0; v < h; v++) {
		for (int u = 0; u < w; u++) {
			temp[v * w + u] = pix[(h - 1 - v) * w + u];
		}
	}

	TIFF* tiff = TIFFOpen(filename, "w");
	if (tiff == NULL) return false;

	TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, w);
	TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, h);
	TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, sizeof(unsigned int));
	TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8);
	//TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_BOTLEFT);
	TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

	TIFFWriteEncodedStrip(tiff, 0, temp, w * h * sizeof(unsigned int));

	TIFFClose(tiff);

	delete [] temp;

	return true;
}

void FrameBuffer::Draw2DBigPoint(int u0, int v0, int psize, const V3 &color, float z) {
	for (int v = v0-psize/2; v <= v0+psize/2; v++) {
		for (int u = u0-psize/2; u <= u0+psize/2; u++) {
			SetGuarded(u, v, color.GetColor(), z);
		}
	}
}

void FrameBuffer::Draw3DBigPoint(PPC* ppc, const V3 &p, int psize, const V3 &color) {
	V3 pp;
	if (!ppc->Project(p, pp)) return;

	Draw2DBigPoint(pp.x(), pp.y(), psize, color, pp.z());
}

bool FrameBuffer::isHidden(int u, int v, float z) {
	if (u < 0 || u >= w || v < 0 || v >= h) return true;


	float z2 = zb[(h-1-v)*w+u];

	if (zb[(h-1-v)*w+u] >= z) return true;
	else return false;
}

bool FrameBuffer::isCompletelyHidden(float u, float v, float z) {
	int uu0 = (int)(u - 0.5f);
	int uu1 = uu0 + 1;
	int vv0 = (int)(v - 0.5f);
	int vv1 = vv0 + 1;

	z *= 1.01f;

	if (isHidden(uu0, vv0, z) && isHidden(uu1, vv0, z) && isHidden(uu0, vv1, z) && isHidden(uu1, vv1, z)) return true;
	else return false;
}

bool FrameBuffer::isPartiallyHidden(float u, float v, float z) {
	int uu0 = (int)(u - 0.5f);
	int uu1 = uu0 + 1;
	int vv0 = (int)(v - 0.5f);
	int vv1 = vv0 + 1;

	z *= 1.01f;

	if (isHidden(uu0, vv0, z) || isHidden(uu1, vv0, z) || isHidden(uu0, vv1, z) || isHidden(uu1, vv1, z)) return true;
	else return false;
}

void FrameBuffer::rasterize(PPC* ppc, const M33 &camMat, const Vertex &p0, const Vertex &p1, const Vertex &p2) {
	AABB box;

	// if one of the vertices are behind the camera, skip this triangle
	if (p0.pv.x() == std::numeric_limits<float>::max() ||
		p1.pv.x() == std::numeric_limits<float>::max() ||
		p2.pv.x() == std::numeric_limits<float>::max()) return;

	// compute the bounding box
	box.AddPoint(p0.pv);
	box.AddPoint(p1.pv);
	box.AddPoint(p2.pv);

	// the bounding box should be inside the screen
	int u_min = (int)box.minCorner().x();
	if (u_min < 0) u_min = 0;;
	int u_max = (int)box.maxCorner().x();
	if (u_max >= w) u_max = w - 1;
	int v_min = (int)box.minCorner().y();
	if (v_min < 0) v_min = 0;
	int v_max = (int)box.maxCorner().y();
	if (v_max >= h) v_max = h - 1;

	// setup some variables
	float denom = ((p1.pv - p0.pv) ^ (p2.pv - p0.pv)).z();
	M33 Q;
	Q.SetColumn(0, p0.v - ppc->C);
	Q.SetColumn(1, p1.v - ppc->C);
	Q.SetColumn(2, p2.v - ppc->C);
	Q = Q.Inverted() * camMat;

	for (int u = u_min; u <= u_max; u++) {
		for (int v = v_min; v <= v_max; v++) {
			V3 pp((u + 0.5f) * ppc->a.Length(), (v + 0.5f) * ppc->b.Length(), 0.0f);

			float s = ((pp - p0.pv) ^ (p2.pv - p0.pv)).z() / denom;
			float t = -((pp - p0.pv) ^ (p1.pv - p0.pv)).z() / denom;

			// if the point is outside the triangle, skip it.
			if (s < 0 || s > 1 || t < 0 || t > 1 || s + t > 1) continue;
			//if (s < -0.01f || s > 1.01f || t < -0.01f || t > 1.01f || s + t > 1.01f) continue;
				
			V3 a = Q * V3((float)u + 0.5f, (float)v + 0.5f, 1.0f);
			float w2 = a.x() + a.y() + a.z();
			float s2 = a.y() / w2;
			float t2 = a.z() / w2;

			// locate the corresponding point in the model space.
			V3 p = p0.v * (1 - s2 - t2) + p1.v * s2 + p2.v * t2;

			// if the point is behind the camera, skip this pixel.
			if (!ppc->Project(p, pp)) continue;

			// check if the point is occluded by other triangles.
			if (zb[(h-1-v)*w+u] >= pp.z()) continue;
			
			V3 c1 = GetColor(ppc, p, p0, p1, p2, s2, t2);

			// draw the pixel (u,v) with the interpolated color.
			Set(u, v, c1.GetColor(), pp.z());
		}
	}
}

V3 FrameBuffer::GetColor(PPC* ppc, const V3 &p, const Vertex &p0, const Vertex &p1, const Vertex &p2, float s, float t) {
	// interpolate the color
	V3 c = p0.c * (1.0f - s - t) + p1.c * s + p2.c * t;

	// interpolate the normal for Phong shading
	V3 n = p0.n * (1.0f - s - t) + p1.n * s + p2.n * t;
	V3 c1;
	for (int i = 0; i < scene->lights->size(); i++) {
		c1 = c1 + scene->lights->at(i)->GetColor(ppc, p, c, n);
	}
	if (c1.x() > 1.0f) c1[0] = 1.0f;
	if (c1.y() > 1.0f) c1[1] = 1.0f;
	if (c1.z() > 1.0f) c1[2] = 1.0f;

	return c1;
}

void FrameBuffer::rasterizeShadow(PPC* ppc, const M33 &camMat, const Vertex &p0, const Vertex &p1, const Vertex &p2) {
	AABB box;

	// if one of the vertices are behind the camera, skip this triangle
	if (p0.pv.x() == std::numeric_limits<float>::max() ||
		p1.pv.x() == std::numeric_limits<float>::max() ||
		p2.pv.x() == std::numeric_limits<float>::max()) return;

	// compute the bounding box
	box.AddPoint(p0.pv);
	box.AddPoint(p1.pv);
	box.AddPoint(p2.pv);

	// the bounding box should be inside the screen
	int u_min = (int)box.minCorner().x();
	if (u_min < 0) u_min = 0;;
	int u_max = (int)box.maxCorner().x();
	if (u_max >= w) u_max = w - 1;
	int v_min = (int)box.minCorner().y();
	if (v_min < 0) v_min = 0;
	int v_max = (int)box.maxCorner().y();
	if (v_max >= h) v_max = h - 1;

	// setup some variables
	float denom = ((p1.pv - p0.pv) ^ (p2.pv - p0.pv)).z();
	M33 Q;
	Q.SetColumn(0, p0.v - ppc->C);
	Q.SetColumn(1, p1.v - ppc->C);
	Q.SetColumn(2, p2.v - ppc->C);
	Q = Q.Inverted() * camMat;

	for (int u = u_min; u <= u_max; u++) {
		for (int v = v_min; v <= v_max; v++) {
			V3 pp((u + 0.5f) * ppc->a.Length(), (v + 0.5f) * ppc->b.Length(), 0.0f);

			float s = ((pp - p0.pv) ^ (p2.pv - p0.pv)).z() / denom;
			float t = -((pp - p0.pv) ^ (p1.pv - p0.pv)).z() / denom;

			// if the point is outside the triangle, skip it.
			if (s < 0 || s > 1 || t < 0 || t > 1 || s + t > 1) continue;
				
			V3 a = Q * V3((float)u + 0.5f, (float)v + 0.5f, 1.0f);
			float w2 = a.x() + a.y() + a.z();
			float s2 = a.y() / w2;
			float t2 = a.z() / w2;

			// locate the corresponding point on the triangle plane.
			V3 p = p0.v * (1 - s2 - t2) + p1.v * s2 + p2.v * t2;

			// if the point is behind the camera, skip this pixel.
			if (!ppc->Project(p, pp)) continue;

			// check if the point is occluded by other triangles.
			if (zb[(h-1-v)*w+u] >= pp.z()) continue;		

			V3 c1 = GetColor(ppc, p, p0, p1, p2, s2, t2);
			c1 = c1 * 0.1f;

			// draw the pixel (u,v) with the interpolated color.
			Set(u, v, c1.GetColor(), pp.z());
		}
	}
}

void FrameBuffer::rasterizeWithTexture(PPC* ppc, const M33 &camMat, const Vertex &p0, const Vertex &p1, const Vertex &p2, Texture* texture) {
	AABB box;

	// if one of the vertices are behind the camera, skip this triangle
	if (p0.pv.x() == std::numeric_limits<float>::max() ||
		p1.pv.x() == std::numeric_limits<float>::max() ||
		p2.pv.x() == std::numeric_limits<float>::max()) return;

	// compute the bounding box
	box.AddPoint(p0.pv);
	box.AddPoint(p1.pv);
	box.AddPoint(p2.pv);

	// the bounding box should be inside the screen
	int u_min = (int)(box.minCorner().x() + 0.5f);
	if (u_min < 0) u_min = 0;;
	if (u_min > w) return;
	int u_max = (int)(box.maxCorner().x() - 0.5f);
	if (u_max >= w) u_max = w - 1;
	if (u_max < 0) return;
	int v_min = (int)(box.minCorner().y() + 0.5f);
	if (v_min < 0) v_min = 0;
	if (v_min > h) return;
	int v_max = (int)(box.maxCorner().y() - 0.5f);
	if (v_max >= h) v_max = h - 1;
	if (v_max < 0) return;

	// the bounding box of texture coordinate
	AABB boxTexCoord;
	boxTexCoord.AddPoint(V3(p0.t[0], p0.t[1], 0.0f));
	boxTexCoord.AddPoint(V3(p1.t[0], p1.t[1], 0.0f));
	boxTexCoord.AddPoint(V3(p2.t[0], p2.t[1], 0.0f));

	// set the mipmap according to the AABB
	texture->SetMipMap(u_max - u_min, v_max - v_min, boxTexCoord.maxCorner().x() - boxTexCoord.minCorner().x(), boxTexCoord.maxCorner().y() - boxTexCoord.minCorner().y());

	// setup some variables
	float denom = ((p1.pv - p0.pv) ^ (p2.pv - p0.pv)).z();
	M33 Q;
	Q.SetColumn(0, p0.v - ppc->C);
	Q.SetColumn(1, p1.v - ppc->C);
	Q.SetColumn(2, p2.v - ppc->C);
	Q = Q.Inverted() * camMat;

	for (int u = u_min; u <= u_max; u++) {
		for (int v = v_min; v <= v_max; v++) {
			V3 pp((u + 0.5f) * ppc->a.Length(), (v + 0.5f) * ppc->b.Length(), 0.0f);

			float s = ((pp - p0.pv) ^ (p2.pv - p0.pv)).z() / denom;
			float t = -((pp - p0.pv) ^ (p1.pv - p0.pv)).z() / denom;

			// if the point is outside the triangle, skip it.
			if (s < 0 || s > 1 || t < 0 || t > 1 || s + t > 1) continue;
				
			V3 a = Q * V3((float)u + 0.5f, (float)v + 0.5f, 1.0f);
			float w2 = a.x() + a.y() + a.z();
			float s2 = a.y() / w2;
			float t2 = a.z() / w2;

			// unproject and locate the corresponding point on the triangle plane.
			V3 p = p0.v * (1 - s2 - t2) + p1.v * s2 + p2.v * t2;

			// if the point is behind the camera, skip this pixel.
			if (!ppc->Project(p, pp)) continue;

			// check if the point is occluded by other triangles.
			if (zb[(h-1-v)*w+u] >= pp.z()) continue;

			V3 c1 = GetColorFromTexture(ppc, p, p0, p1, p2, texture, s2, t2);
			/*
			// get the corresponding color by using bi-linear interpolation lookup
			float t_x = p0.t[0] * (1.0f - s2 - t2) + p1.t[0] * s2 + p2.t[0] * t2;
			float t_y = p0.t[1] * (1.0f - s2 - t2) + p1.t[1] * s2 + p2.t[1] * t2;

			V3 c = texture->GetColor(t_x, t_y);

			// interpolate the normal for Phong shading
			V3 n = p0.n * (1.0f - s2 - t2) + p1.n * s2 + p2.n * t2;
			V3 c1;
			for (int i = 0; i < scene->lights.size(); i++) {
				c1 = c1 + scene->lights[i]->GetColor(ppc, p, c, n);
			}
			if (c1.x() > 1.0f) c1[0] = 1.0f;
			if (c1.y() > 1.0f) c1[1] = 1.0f;
			if (c1.z() > 1.0f) c1[2] = 1.0f;
			*/

			// draw the pixel (u,v) with the interpolated color.
			Set(u, v, c1.GetColor(), pp.z());
		}
	}
}

V3 FrameBuffer::GetColorFromTexture(PPC* ppc, const V3 &p, const Vertex &p0, const Vertex &p1, const Vertex &p2, Texture* texture, float s, float t) {
	// get the corresponding color by using bi-linear interpolation lookup
	float t_x = p0.t[0] * (1.0f - s - t) + p1.t[0] * s + p2.t[0] * t;
	float t_y = p0.t[1] * (1.0f - s - t) + p1.t[1] * s + p2.t[1] * t;

	V3 c = texture->GetColor(t_x, t_y);

	// interpolate the normal for Phong shading
	V3 n = p0.n * (1.0f - s - t) + p1.n * s + p2.n * t;
	V3 c1;
	for (int i = 0; i < scene->lights->size(); i++) {
		c1 = c1 + scene->lights->at(i)->GetColor(ppc, p, c, n);
	}
	if (c1.x() > 1.0f) c1[0] = 1.0f;
	if (c1.y() > 1.0f) c1[1] = 1.0f;
	if (c1.z() > 1.0f) c1[2] = 1.0f;

	return c1;
}

void FrameBuffer::rasterizeWithCubeMap(PPC* ppc, const M33 &camMat, const Vertex &p0, const Vertex &p1, const Vertex &p2, CubeMap* cubeMap) {
	AABB box;

	// if one of the vertices are behind the camera, skip this triangle
	if (p0.pv.x() == std::numeric_limits<float>::max() ||
		p1.pv.x() == std::numeric_limits<float>::max() ||
		p2.pv.x() == std::numeric_limits<float>::max()) return;

	// compute the bounding box
	box.AddPoint(p0.pv);
	box.AddPoint(p1.pv);
	box.AddPoint(p2.pv);

	// the bounding box should be inside the screen
	int u_min = (int)(box.minCorner().x() + 0.5f);
	if (u_min < 0) u_min = 0;;
	if (u_min > w) return;
	int u_max = (int)(box.maxCorner().x() - 0.5f);
	if (u_max >= w) u_max = w - 1;
	if (u_max < 0) return;
	int v_min = (int)(box.minCorner().y() + 0.5f);
	if (v_min < 0) v_min = 0;
	if (v_min > h) return;
	int v_max = (int)(box.maxCorner().y() - 0.5f);
	if (v_max >= h) v_max = h - 1;
	if (v_max < 0) return;

	// the bounding box of texture coordinate
	AABB boxTexCoord;
	int face = cubeMap->GetFace(((p0.v - ppc->C).reflect(p0.n) + (p1.v - ppc->C).reflect(p1.n) + (p2.v - ppc->C).reflect(p2.n)) / 3.0f);
	boxTexCoord.AddPoint(cubeMap->GetTextureCoord(face, (p0.v - ppc->C).reflect(p0.n)));
	boxTexCoord.AddPoint(cubeMap->GetTextureCoord(face, (p1.v - ppc->C).reflect(p1.n)));
	boxTexCoord.AddPoint(cubeMap->GetTextureCoord(face, (p2.v - ppc->C).reflect(p2.n)));

	// set the mipmap according to the AABB
	cubeMap->SetMipMap(u_max - u_min, v_max - v_min, boxTexCoord.maxCorner().x() - boxTexCoord.minCorner().x(), boxTexCoord.maxCorner().y() - boxTexCoord.minCorner().y());

	// setup some variables
	float denom = ((p1.pv - p0.pv) ^ (p2.pv - p0.pv)).z();
	M33 Q;
	Q.SetColumn(0, p0.v - ppc->C);
	Q.SetColumn(1, p1.v - ppc->C);
	Q.SetColumn(2, p2.v - ppc->C);
	Q = Q.Inverted() * camMat;

	for (int u = u_min; u <= u_max; u++) {
		for (int v = v_min; v <= v_max; v++) {
			V3 pp((u + 0.5f) * ppc->a.Length(), (v + 0.5f) * ppc->b.Length(), 0.0f);

			float s = ((pp - p0.pv) ^ (p2.pv - p0.pv)).z() / denom;
			float t = -((pp - p0.pv) ^ (p1.pv - p0.pv)).z() / denom;

			// if the point is outside the triangle, skip it.
			if (s < 0 || s > 1 || t < 0 || t > 1 || s + t > 1) continue;
				
			V3 a = Q * V3((float)u + 0.5f, (float)v + 0.5f, 1.0f);
			float w2 = a.x() + a.y() + a.z();
			float s2 = a.y() / w2;
			float t2 = a.z() / w2;

			// unproject and locate the corresponding point on the triangle plane.
			V3 p = p0.v * (1 - s2 - t2) + p1.v * s2 + p2.v * t2;

			// if the point is behind the camera, skip this pixel.
			if (!ppc->Project(p, pp)) continue;

			// check if the point is occluded by other triangles.
			if (zb[(h-1-v)*w+u] >= pp.z()) continue;

			V3 c1 = GetColorFromCubeMap(ppc, p, p0, p1, p2, cubeMap, s, t);

			// draw the pixel (u,v) with the color.
			Set(u, v, c1.GetColor(), pp.z());
		}
	}
}

V3 FrameBuffer::GetColorFromCubeMap(PPC* ppc, const V3 &p, const Vertex &p0, const Vertex &p1, const Vertex &p2, CubeMap* cubeMap, float s, float t) {
	// interpolate the normal
	V3 n = p0.n * (1.0f - s - t) + p1.n * s + p2.n * t;

	// get the color by using the cube map
	V3 c = cubeMap->lookup((p - ppc->C).reflect(n));

	// apply Phong shading
	V3 c1;
	for (int i = 0; i < scene->lights->size(); i++) {
		c1 = c1 + scene->lights->at(i)->GetColor(ppc, p, c, n);
	}
	if (c1.x() > 1.0f) c1[0] = 1.0f;
	if (c1.y() > 1.0f) c1[1] = 1.0f;
	if (c1.z() > 1.0f) c1[2] = 1.0f;

	return c1;
}

void FrameBuffer::rasterizeByRayTracing(PPC* ppc, const M33 &camMat, const Vertex &p0, const Vertex &p1, const Vertex &p2, CubeMap* cubeMap, Texture* texture) {
	AABB box;

	// if one of the vertices are behind the camera, skip this triangle
	if (p0.pv.x() == std::numeric_limits<float>::max() ||
		p1.pv.x() == std::numeric_limits<float>::max() ||
		p2.pv.x() == std::numeric_limits<float>::max()) return;

	// compute the bounding box
	box.AddPoint(p0.pv);
	box.AddPoint(p1.pv);
	box.AddPoint(p2.pv);

	// the bounding box should be inside the screen
	int u_min = (int)(box.minCorner().x() + 0.5f);
	if (u_min < 0) u_min = 0;;
	if (u_min > w) return;
	int u_max = (int)(box.maxCorner().x() - 0.5f);
	if (u_max >= w) u_max = w - 1;
	if (u_max < 0) return;
	int v_min = (int)(box.minCorner().y() + 0.5f);
	if (v_min < 0) v_min = 0;
	if (v_min > h) return;
	int v_max = (int)(box.maxCorner().y() - 0.5f);
	if (v_max >= h) v_max = h - 1;
	if (v_max < 0) return;

	// setup some variables
	float denom = ((p1.pv - p0.pv) ^ (p2.pv - p0.pv)).z();
	M33 Q;
	Q.SetColumn(0, p0.v - ppc->C);
	Q.SetColumn(1, p1.v - ppc->C);
	Q.SetColumn(2, p2.v - ppc->C);
	Q = Q.Inverted() * camMat;

	for (int u = u_min; u <= u_max; u++) {
		for (int v = v_min; v <= v_max; v++) {
			V3 pp((u + 0.5f) * ppc->a.Length(), (v + 0.5f) * ppc->b.Length(), 0.0f);

			float s = ((pp - p0.pv) ^ (p2.pv - p0.pv)).z() / denom;
			float t = -((pp - p0.pv) ^ (p1.pv - p0.pv)).z() / denom;

			// if the point is outside the triangle, skip it.
			if (s < 0 || s > 1 || t < 0 || t > 1 || s + t > 1) continue;
				
			V3 a = Q * V3((float)u + 0.5f, (float)v + 0.5f, 1.0f);
			float w2 = a.x() + a.y() + a.z();
			float s2 = a.y() / w2;
			float t2 = a.z() / w2;

			// unproject and locate the corresponding point on the triangle plane.
			V3 p = p0.v * (1 - s2 - t2) + p1.v * s2 + p2.v * t2;

			// if the point is behind the camera, skip this pixel.
			if (!ppc->Project(p, pp)) continue;

			// check if the point is occluded by other triangles.
			if (zb[(h-1-v)*w+u] >= pp.z()) continue;

			// interpolate the normal
			V3 n = p0.n * (1.0f - s2 - t2) + p1.n * s2 + p2.n * t2;

			// get the color by using the ray tracing
			float dist;
			V3 c = scene->RayTrace(ppc, p, (p - ppc->C).reflect(n), dist);
			if (dist == std::numeric_limits<float>::max()) {
				if (cubeMap != NULL) {
					c = GetColorFromCubeMap(ppc, p, p0, p1, p2, cubeMap, s2, t2);
				} else if (texture != NULL) {
					c = GetColorFromTexture(ppc, p, p0, p1, p2, texture, s2, t2);
				} else {
					c = GetColor(ppc, p, p0, p1, p2, s2, t2);
				}
			}

			// apply Phong shading
			V3 c1;
			for (int i = 0; i < scene->lights->size(); i++) {
				c1 = c1 + scene->lights->at(i)->GetColor(ppc, p, c, n);
			}
			if (c1.x() > 1.0f) c1[0] = 1.0f;
			if (c1.y() > 1.0f) c1[1] = 1.0f;
			if (c1.z() > 1.0f) c1[2] = 1.0f;

			// draw the pixel (u,v) with the color.
			Set(u, v, c1.GetColor(), pp.z());
		}
	}
}

/**
 * Projective texture mapping.
 *
 * @param ppc			the desired camera
 * @param fb0			the reference image plane
 */
void FrameBuffer::RasterizeProjectiveTextureMapping(PPC* ppc, PPC* ppc0, FrameBuffer* fb0, const M33 &camMat, const V3 &Q1, const M33 &Q2, const Vertex &p0, const Vertex &p1, const Vertex &p2) {
	AABB box;

	// if one of the vertices are behind the camera, skip this triangle
	if (p0.pv.x() == std::numeric_limits<float>::max() ||
		p1.pv.x() == std::numeric_limits<float>::max() ||
		p2.pv.x() == std::numeric_limits<float>::max()) return;

	// compute the bounding box
	box.AddPoint(p0.pv);
	box.AddPoint(p1.pv);
	box.AddPoint(p2.pv);

	// the bounding box should be inside the screen
	int u_min = (int)box.minCorner().x();
	if (u_min < 0) u_min = 0;
	int u_max = (int)box.maxCorner().x();
	if (u_max >= w) u_max = w - 1;
	int v_min = (int)box.minCorner().y();
	if (v_min < 0) v_min = 0;
	int v_max = (int)box.maxCorner().y();
	if (v_max >= h) v_max = h - 1;

	// setup some variables
	float denom = ((p1.pv - p0.pv) ^ (p2.pv - p0.pv)).z();
	M33 Q;
	Q.SetColumn(0, p0.v - ppc->C);
	Q.SetColumn(1, p1.v - ppc->C);
	Q.SetColumn(2, p2.v - ppc->C);
	Q = Q.Inverted() * camMat;

	for (int u = u_min; u <= u_max; u++) {
		for (int v = v_min; v <= v_max; v++) {
			V3 pp((u + 0.5f) * ppc->a.Length(), (v + 0.5f) * ppc->b.Length(), 0.0f);

			float s = ((pp - p0.pv) ^ (p2.pv - p0.pv)).z() / denom;
			float t = -((pp - p0.pv) ^ (p1.pv - p0.pv)).z() / denom;

			// if the point is outside the triangle, skip it.
			if (s < 0 || s > 1 || t < 0 || t > 1 || s + t > 1) continue;
				
			V3 a = Q * V3((float)u + 0.5f, (float)v + 0.5f, 1.0f);
			float w2 = a.x() + a.y() + a.z();
			float s2 = a.y() / w2;
			float t2 = a.z() / w2;

			// locate the corresponding point in the model space.
			V3 p = p0.v * (1 - s2 - t2) + p1.v * s2 + p2.v * t2;

			// if the point is behind the camera, skip this pixel.
			if (!ppc->Project(p, pp)) continue;

			// check if the point is occluded by other triangles.
			if (zb[(h-1-v)*w+u] >= pp.z()) continue;
			
			V3 pp1((float)u + 0.5f, (float)v + 0.5f, 1.0f);
			pp1 = pp1 / pp.z();
			//	(((float)u + 0.5f) / pp.z(), ((float)v + 0.5f) / pp.z(), 1.0f / pp.z());
			
			V3 c1;

			// get the reference point on the reference image plane
			V3 pp0 = Q1 + Q2 * pp1;
			if (pp0.z() > 0.0f) {	// the 3D point is in front of the reference camera
				pp0[0] /= pp0.z();
				pp0[1] /= pp0.z();
				pp0[2] = 1.0f / pp0.z();

				if (pp0.x() >= 0 && pp0.x() < fb0->w && pp0.y() >= 0 && pp0.y() < fb0->h) {
					if (fb0->GetZ(pp0.x(), pp0.y()) > 0.0f) {
						c1 = fb0->Get(pp0.x(), pp0.y());
					} else {
						// This point is a background pixel in the reference image,
						// so just draw the model's color
						//c1 = p0.c * (1.0f - s2 - t2) + p1.c * s2 + p2.c * t2;
						c1 = GetColor(ppc, p, p0, p1, p2, s2, t2);
					}
				} else {
					// draw the original color
					//c1 = p0.c * (1.0f - s2 - t2) + p1.c * s2 + p2.c * t2;
					c1 = GetColor(ppc, p, p0, p1, p2, s2, t2);
				}
			} else {				// the 3D point is behind the reference camera
				// draw the original color
				//c1 = p0.c * (1.0f - s2 - t2) + p1.c * s2 + p2.c * t2;
				c1 = GetColor(ppc, p, p0, p1, p2, s2, t2);
			}

			// draw the pixel (u,v) with the interpolated color.
			Set(u, v, c1.GetColor(), pp.z());
		}
	}
}

/**
 * Shadow mapping.
 *
 * @param ppc			the desired camera
 * @param fb0			the reference image plane
 */
void FrameBuffer::RasterizeShadowMapping(PPC* ppc, PPC* ppc0, FrameBuffer* fb0, const M33 &camMat, const V3 &Q1, const M33 &Q2, const Vertex &p0, const Vertex &p1, const Vertex &p2) {
	AABB box;

	// if one of the vertices are behind the camera, skip this triangle
	if (p0.pv.x() == std::numeric_limits<float>::max() ||
		p1.pv.x() == std::numeric_limits<float>::max() ||
		p2.pv.x() == std::numeric_limits<float>::max()) return;

	// compute the bounding box
	box.AddPoint(p0.pv);
	box.AddPoint(p1.pv);
	box.AddPoint(p2.pv);

	// the bounding box should be inside the screen
	int u_min = (int)box.minCorner().x();
	if (u_min < 0) u_min = 0;
	int u_max = (int)box.maxCorner().x();
	if (u_max >= w) u_max = w - 1;
	int v_min = (int)box.minCorner().y();
	if (v_min < 0) v_min = 0;
	int v_max = (int)box.maxCorner().y();
	if (v_max >= h) v_max = h - 1;

	// setup some variables
	float denom = ((p1.pv - p0.pv) ^ (p2.pv - p0.pv)).z();
	M33 Q;
	Q.SetColumn(0, p0.v - ppc->C);
	Q.SetColumn(1, p1.v - ppc->C);
	Q.SetColumn(2, p2.v - ppc->C);
	Q = Q.Inverted() * camMat;

	for (int u = u_min; u <= u_max; u++) {
		for (int v = v_min; v <= v_max; v++) {
			V3 pp((u + 0.5f) * ppc->a.Length(), (v + 0.5f) * ppc->b.Length(), 0.0f);

			float s = ((pp - p0.pv) ^ (p2.pv - p0.pv)).z() / denom;
			float t = -((pp - p0.pv) ^ (p1.pv - p0.pv)).z() / denom;

			// if the point is outside the triangle, skip it.
			if (s < 0 || s > 1 || t < 0 || t > 1 || s + t > 1) continue;
				
			V3 a = Q * V3((float)u + 0.5f, (float)v + 0.5f, 1.0f);
			float w2 = a.x() + a.y() + a.z();
			float s2 = a.y() / w2;
			float t2 = a.z() / w2;

			// locate the corresponding point on the triangle plane.
			V3 p = p0.v * (1 - s2 - t2) + p1.v * s2 + p2.v * t2;

			// if the point is behind the camera, skip this pixel.
			if (!ppc->Project(p, pp)) continue;

			// check if the point is occluded by other triangles.
			if (zb[(h-1-v)*w+u] >= pp.z()) continue;
			
			V3 pp1((float)u + 0.5f, (float)v + 0.5f, 1.0f);
			pp1 = pp1 / pp.z();
			//	(((float)u + 0.5f) / pp.z(), ((float)v + 0.5f) / pp.z(), 1.0f / pp.z());
			
			V3 c1;

			// get the reference point on the reference image plane
			V3 pp0 = Q1 + Q2 * pp1;
			if (pp0.z() > 0.0f) {	// the 3D point is in front of the reference camera
				pp0[0] /= pp0.z();
				pp0[1] /= pp0.z();
				pp0[2] = 1.0f / pp0.z();

				if (fb0->isCompletelyHidden(pp0.x(), pp0.y(), pp0.z())) {
					// draw a shadow
					c1 = p0.c * (1.0f - s2 - t2) + p1.c * s2 + p2.c * t2;
					c1 = c1 * 0.2f;
				} else if (fb0->isPartiallyHidden(pp0.x(), pp0.y(), pp0.z())) {
					// draw a shadow?
					c1 = p0.c * (1.0f - s2 - t2) + p1.c * s2 + p2.c * t2;
					c1 = c1 * 0.2f;
				} else {
					c1 = p0.c * (1.0f - s2 - t2) + p1.c * s2 + p2.c * t2;
				}
			} else {				// the 3D point is behind the reference camera
				// draw a shadow
				c1 = p0.c * (1.0f - s2 - t2) + p1.c * s2 + p2.c * t2;
				c1 = c1 * 0.2f;
			}

			// draw the pixel (u,v) with the interpolated color.
			Set(u, v, c1.GetColor(), pp.z());
		}
	}
}

/**
 * Rasterize with projector.
 *
 * @param ppc			the desired camera
 * @param projector		the projector
 * @param cammat		camera matrix
 * @param Q1			Q1 matrix
 * @param Q2			Q2 matrix
 * @param p0			the point 0 of the triangle
 * @param p1			the point 1 of the triangle
 * @param p2			the point 2 of the triangle
 */
void FrameBuffer::rasterizeWithProjector(PPC* ppc, Projector* projector, const M33 &camMat, const V3 &Q1, const M33 &Q2, const Vertex &p0, const Vertex &p1, const Vertex &p2) {
	AABB box;

	// if one of the vertices are behind the camera, skip this triangle
	if (p0.pv.x() == std::numeric_limits<float>::max() ||
		p1.pv.x() == std::numeric_limits<float>::max() ||
		p2.pv.x() == std::numeric_limits<float>::max()) return;

	// compute the bounding box
	box.AddPoint(p0.pv);
	box.AddPoint(p1.pv);
	box.AddPoint(p2.pv);

	// the bounding box should be inside the screen
	int u_min = (int)box.minCorner().x();
	if (u_min < 0) u_min = 0;
	int u_max = (int)box.maxCorner().x();
	if (u_max >= w) u_max = w - 1;
	int v_min = (int)box.minCorner().y();
	if (v_min < 0) v_min = 0;
	int v_max = (int)box.maxCorner().y();
	if (v_max >= h) v_max = h - 1;

	// setup some variables
	float denom = ((p1.pv - p0.pv) ^ (p2.pv - p0.pv)).z();
	M33 Q;
	Q.SetColumn(0, p0.v - ppc->C);
	Q.SetColumn(1, p1.v - ppc->C);
	Q.SetColumn(2, p2.v - ppc->C);
	Q = Q.Inverted() * camMat;

	for (int u = u_min; u <= u_max; u++) {
		for (int v = v_min; v <= v_max; v++) {
			V3 pp((u + 0.5f) * ppc->a.Length(), (v + 0.5f) * ppc->b.Length(), 0.0f);

			float s = ((pp - p0.pv) ^ (p2.pv - p0.pv)).z() / denom;
			float t = -((pp - p0.pv) ^ (p1.pv - p0.pv)).z() / denom;

			// if the point is outside the triangle, skip it.
			if (s < 0 || s > 1 || t < 0 || t > 1 || s + t > 1) continue;
				
			V3 a = Q * V3((float)u + 0.5f, (float)v + 0.5f, 1.0f);
			float w2 = a.x() + a.y() + a.z();
			float s2 = a.y() / w2;
			float t2 = a.z() / w2;

			// locate the corresponding point in the model space.
			V3 p = p0.v * (1 - s2 - t2) + p1.v * s2 + p2.v * t2;

			// if the point is behind the camera, skip this pixel.
			if (!ppc->Project(p, pp)) continue;

			// check if the point is occluded by other triangles.
			if (zb[(h-1-v)*w+u] >= pp.z()) continue;
			
			V3 pp1((float)u + 0.5f, (float)v + 0.5f, 1.0f);
			pp1 = pp1 / pp.z();
			//	(((float)u + 0.5f) / pp.z(), ((float)v + 0.5f) / pp.z(), 1.0f / pp.z());
			
			V3 c1;

			// get the reference point on the reference image plane
			V3 pp0 = Q1 + Q2 * pp1;
			if (pp0.z() > 0.0f) {	// the 3D point is in front of the reference camera
				pp0[0] /= pp0.z();
				pp0[1] /= pp0.z();
				pp0[2] = 1.0f / pp0.z();

				if (pp0.x() >= 0 && pp0.x() < projector->fb0->w && pp0.y() >= 0 && pp0.y() < projector->fb0->h) {
					if (projector->fb0->GetZ(pp0.x(), pp0.y()) > 0.0f) {
						if (projector->fb1->isCompletelyHidden(pp0.x(), pp0.y(), pp0.z())) {
							// This point is ocluded in the reference image,
							// so just draw the model's color.
							c1 = GetColor(ppc, p, p0, p1, p2, s2, t2);
							projector->fb1->isCompletelyHidden(pp0.x(), pp0.y(), pp0.z());
						} else {
							c1 = projector->fb0->Get(pp0.x(), pp0.y());
						}
					} else {
						// This point is a background pixel in the reference image,
						// so just draw the model's color.
						c1 = GetColor(ppc, p, p0, p1, p2, s2, t2);
					}
				} else {
					// This point is out of the frustrum of the reference image,
					// so just draw the original color.
					c1 = GetColor(ppc, p, p0, p1, p2, s2, t2);
				}
			} else {	
				// The 3D point is behind the reference camera,
				// so just draw the original color.
				c1 = GetColor(ppc, p, p0, p1, p2, s2, t2);
			}

			// draw the pixel (u,v) with the interpolated color.
			Set(u, v, c1.GetColor(), pp.z());
		}
	}
}

V3 FrameBuffer::Get(int u, int v) {
	V3 ret;
	ret.SetColor(pix[(h-1-v)*w+u]);
	return ret;
}

V3 FrameBuffer::Get(float u, float v) {
	V3 ret;

	int u0 = (int)(u - 0.5f);
	int u1 = u0 + 1;
	if (u1 < 0 || u0 >= w) return ret;
	if (u0 < 0) u0 = 0;
	if (u1 >= w) u1 = w - 1;

	int v0 = (int)(v - 0.5f);
	int v1 = v0 + 1;
	if (v1 < 0 || v0 >= h) return ret;
	if (v0 < 0) v0 = 0;
	if (v1 >= h) v1 = h - 1;

	float s = u - (float)u0;
	float t = v - (float)v0;

	return (Get(u0, v0) * (1 - s) + Get(u1, v0) * s) * (1 - t) + (Get(u0, v1) * (1 - s) + Get(u1, v1) * s) * t;
}

V3 FrameBuffer::GetNearestColor(int u, int v) {
	for (int i = 1; i <= 6; i++) {
		for (int u0 = u - i; u0 <= u + i; u0++) {
			if (GetZ(u0, v - i) > 0.0f) {
				return Get(u0, v - i);
			}
		}
		for (int u0 = u - i; u0 <= u + i; u0++) {
			if (GetZ(u0, v + i) > 0.0f) {
				return Get(u0, v + i);
			}
		}
		for (int v0 = v - i; v0 <= v + i; v0++) {
			if (GetZ(u - i, v0) > 0.0f) {
				return Get(u - i, v0);
			}
		}
		for (int v0 = v - i; v0 <= v + i; v0++) {
			if (GetZ(u + i, v0) > 0.0f) {
				return Get(u + i, v0);
			}
		}
	}

	return V3(0.0f, 0.0f, 0.0f);
}

float FrameBuffer::GetZ(int u, int v) {
	return zb[(h-1-v)*w+u];
}