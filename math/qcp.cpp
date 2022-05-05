/*************************************************************************/
/*  qcp.cpp                                                              */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2019 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2019 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "qcp.h"

QCP::QCP(double evec_prec, double eval_prec) {
	this->evec_prec = evec_prec;
	this->eval_prec = eval_prec;
}

void QCP::setMaxIterations(int max) {
	max_iterations = max;
}

void QCP::set(PackedVector3Array &p_target, PackedVector3Array &p_moved) {
	target = p_target;
	moved = p_moved;
	rmsdCalculated = false;
	transformationCalculated = false;
	innerProductCalculated = false;
}

double QCP::getRmsd() {
	if (!rmsdCalculated) {
		calcRmsd(moved, target);
		rmsdCalculated = true;
	}
	return rmsd;
}

Quaternion QCP::getRotation() {
	Quaternion result;
	if (!transformationCalculated) {
		if (!innerProductCalculated) {
			innerProduct(target, moved);
		}
		result = calcRotation();
		transformationCalculated = true;
	}
	return result;
}

void QCP::calcRmsd(double len) {
	if (max_iterations > 0) {
		double Sxx2 = Sxx * Sxx;
		double Syy2 = Syy * Syy;
		double Szz2 = Szz * Szz;

		double Sxy2 = Sxy * Sxy;
		double Syz2 = Syz * Syz;
		double Sxz2 = Sxz * Sxz;

		double Syx2 = Syx * Syx;
		double Szy2 = Szy * Szy;
		double Szx2 = Szx * Szx;

		double SyzSzymSyySzz2 = 2.0 * (Syz * Szy - Syy * Szz);
		double Sxx2Syy2Szz2Syz2Szy2 = Syy2 + Szz2 - Sxx2 + Syz2 + Szy2;

		double c2 = -2.0 * (Sxx2 + Syy2 + Szz2 + Sxy2 + Syx2 + Sxz2 + Szx2 + Syz2 + Szy2);
		double c1 = 8.0 * (Sxx * Syz * Szy + Syy * Szx * Sxz + Szz * Sxy * Syx - Sxx * Syy * Szz - Syz * Szx * Sxy - Szy * Syx * Sxz);

		double Sxy2Sxz2Syx2Szx2 = Sxy2 + Sxz2 - Syx2 - Szx2;

		double c0 = Sxy2Sxz2Syx2Szx2 * Sxy2Sxz2Syx2Szx2 + (Sxx2Syy2Szz2Syz2Szy2 + SyzSzymSyySzz2) * (Sxx2Syy2Szz2Syz2Szy2 - SyzSzymSyySzz2) + (-(SxzpSzx) * (SyzmSzy) + (SxymSyx) * (SxxmSyy - Szz)) * (-(SxzmSzx) * (SyzpSzy) + (SxymSyx) * (SxxmSyy + Szz)) + (-(SxzpSzx) * (SyzpSzy) - (SxypSyx) * (SxxpSyy - Szz)) * (-(SxzmSzx) * (SyzmSzy) - (SxypSyx) * (SxxpSyy + Szz)) + (+(SxypSyx) * (SyzpSzy) + (SxzpSzx) * (SxxmSyy + Szz)) * (-(SxymSyx) * (SyzmSzy) + (SxzpSzx) * (SxxpSyy + Szz)) + (+(SxypSyx) * (SyzmSzy) + (SxzmSzx) * (SxxmSyy - Szz)) * (-(SxymSyx) * (SyzpSzy) + (SxzmSzx) * (SxxpSyy - Szz));

		int i;
		for (i = 1; i < (max_iterations + 1); ++i) {
			double oldg = mxEigenV;
			double Y = 1 / mxEigenV;
			double Y2 = Y * Y;
			double delta = ((((Y * c0 + c1) * Y + c2) * Y2 + 1) / ((Y * c1 + 2 * c2) * Y2 * Y + 4));
			mxEigenV -= delta;

			if (Math::abs(mxEigenV - oldg) < Math::abs(eval_prec * mxEigenV)) {
				break;
			}
		}
	}

	rmsd = Math::sqrt(Math::abs(2.0f * (e0 - mxEigenV) / len));
}

Quaternion QCP::calcRotation() {
	// QCP doesn't handle single targets, so if we only have one point and one
	// target, we just rotate by the angular distance between them
	if (moved.size() == 1) {
		Vector3 u = moved[0];
		Vector3 v = target[0];
		double normProduct = u.length() * v.length();
		if (normProduct == 0.0) {
			return Quaternion();
		}
		double dot = u.dot(v);
		if (dot < ((2.0e-15 - 1.0) * normProduct)) {
			// The special case: u = -v,
			// We select a PI angle rotation around
			// an arbitrary vector orthogonal to u.
			Vector3 w = u.normalized();
			Vector3 axis = getAxis(-w.x, -w.y, -w.z, 0.0f);
			real_t angle = getAngle(-w.x, -w.y, -w.z, 0.0f);
			return Quaternion(axis, angle);
		}
		// The general case: (u, v) defines a plane, we select
		// the shortest possible rotation: axis orthogonal to this plane.
		double q0 = Math::sqrt(0.5 * (1.0 + dot / normProduct));
		double coeff = 1.0 / (2.0 * q0 * normProduct);
		Vector3 q = v.cross(u);
		double q1 = coeff * q.x;
		double q2 = coeff * q.y;
		double q3 = coeff * q.z;
		Vector3 axis = getAxis(q1, q2, q3, q0);
		real_t angle = getAngle(q1, q2, q3, q0);
		return Quaternion(axis, angle);
	} else {
		double a11 = SxxpSyy + Szz - mxEigenV;
		double a12 = SyzmSzy;
		double a13 = -SxzmSzx;
		double a14 = SxymSyx;
		double a21 = SyzmSzy;
		double a22 = SxxmSyy - Szz - mxEigenV;
		double a23 = SxypSyx;
		double a24 = SxzpSzx;
		double a31 = a13;
		double a32 = a23;
		double a33 = Syy - Sxx - Szz - mxEigenV;
		double a34 = SyzpSzy;
		double a41 = a14;
		double a42 = a24;
		double a43 = a34;
		double a44 = Szz - SxxpSyy - mxEigenV;
		double a3344_4334 = a33 * a44 - a43 * a34;
		double a3244_4234 = a32 * a44 - a42 * a34;
		double a3243_4233 = a32 * a43 - a42 * a33;
		double a3143_4133 = a31 * a43 - a41 * a33;
		double a3144_4134 = a31 * a44 - a41 * a34;
		double a3142_4132 = a31 * a42 - a41 * a32;
		double q1 = a22 * a3344_4334 - a23 * a3244_4234 + a24 * a3243_4233;
		double q2 = -a21 * a3344_4334 + a23 * a3144_4134 - a24 * a3143_4133;
		double q3 = a21 * a3244_4234 - a22 * a3144_4134 + a24 * a3142_4132;
		double q4 = -a21 * a3243_4233 + a22 * a3143_4133 - a23 * a3142_4132;

		double qsqr = q1 * q1 + q2 * q2 + q3 * q3 + q4 * q4;

		/*
		 * The following code tries to calculate another column in the adjoint matrix
		 * when the norm of the current column is too small. Usually this commented
		 * block will never be activated. To be absolutely safe this should be
		 * uncommented, but it is most likely unnecessary.
		 */
		if (qsqr < evec_prec) {
			q1 = a12 * a3344_4334 - a13 * a3244_4234 + a14 * a3243_4233;
			q2 = -a11 * a3344_4334 + a13 * a3144_4134 - a14 * a3143_4133;
			q3 = a11 * a3244_4234 - a12 * a3144_4134 + a14 * a3142_4132;
			q4 = -a11 * a3243_4233 + a12 * a3143_4133 - a13 * a3142_4132;
			qsqr = q1 * q1 + q2 * q2 + q3 * q3 + q4 * q4;

			if (qsqr < evec_prec) {
				double a1324_1423 = a13 * a24 - a14 * a23, a1224_1422 = a12 * a24 - a14 * a22;
				double a1223_1322 = a12 * a23 - a13 * a22, a1124_1421 = a11 * a24 - a14 * a21;
				double a1123_1321 = a11 * a23 - a13 * a21, a1122_1221 = a11 * a22 - a12 * a21;

				q1 = a42 * a1324_1423 - a43 * a1224_1422 + a44 * a1223_1322;
				q2 = -a41 * a1324_1423 + a43 * a1124_1421 - a44 * a1123_1321;
				q3 = a41 * a1224_1422 - a42 * a1124_1421 + a44 * a1122_1221;
				q4 = -a41 * a1223_1322 + a42 * a1123_1321 - a43 * a1122_1221;
				qsqr = q1 * q1 + q2 * q2 + q3 * q3 + q4 * q4;

				if (qsqr < evec_prec) {
					q1 = a32 * a1324_1423 - a33 * a1224_1422 + a34 * a1223_1322;
					q2 = -a31 * a1324_1423 + a33 * a1124_1421 - a34 * a1123_1321;
					q3 = a31 * a1224_1422 - a32 * a1124_1421 + a34 * a1122_1221;
					q4 = -a31 * a1223_1322 + a32 * a1123_1321 - a33 * a1122_1221;
					qsqr = q1 * q1 + q2 * q2 + q3 * q3 + q4 * q4;

					if (qsqr < evec_prec) {
						/*
						 * if qsqr is still too small, return the identity rotation
						 */
						return Quaternion();
					}
				}
			}
		}
		real_t min = q1;
		min = q2 < min ? q2 : min;
		min = q3 < min ? q3 : min;
		min = q4 < min ? q4 : min;

		// Copy the axis angle code from the EWBIK.
		real_t q1_m = q1 / min;
		real_t q2_m = q2 / min;
		real_t q3_m = q3 / min;
		real_t q4_m = q4 / min;
		real_t norm = Math::sqrt(q1_m * q1_m + q2_m * q2_m + q3_m * q3_m + q4_m * q4_m);
		Vector3 axis = getAxis(q2_m / norm, q3_m / norm, q4_m / norm, q1_m / norm);
		real_t angle = getAngle(q2_m / norm, q3_m / norm, q4_m / norm, q1_m / norm);
		return Quaternion(axis, angle).normalized();
	}
}

double QCP::getRmsd(PackedVector3Array &moved, PackedVector3Array &fixed) {
	set(moved, fixed);
	return getRmsd();
}

Vector3 QCP::getTranslation() {
	return targetCenter - movedCenter;
}

Vector3 QCP::moveToWeightedCenter(PackedVector3Array &toCenter, Vector<real_t> &weight, Vector3 center) {
	if (!weight.is_empty()) {
		for (int i = 0; i < toCenter.size(); i++) {
			center = toCenter[i] * weight[i];
			wsum += weight[i];
		}

		center /= Vector3(wsum, wsum, wsum);
	} else {
		for (int i = 0; i < toCenter.size(); i++) {
			center += toCenter[i];
			wsum++;
		}
		center /= Vector3(wsum, wsum, wsum);
	}

	return center;
}

void QCP::innerProduct(PackedVector3Array &coords1, PackedVector3Array &coords2) {
	double x1, x2, y1, y2, z1, z2;
	double g1 = 0, g2 = 0;

	Sxx = 0;
	Sxy = 0;
	Sxz = 0;
	Syx = 0;
	Syy = 0;
	Syz = 0;
	Szx = 0;
	Szy = 0;
	Szz = 0;

	if (!weight.is_empty()) {
		// wsum = 0;
		for (int i = 0; i < coords1.size(); i++) {
			// wsum += weight[i];

			x1 = weight[i] * coords1[i].x;
			y1 = weight[i] * coords1[i].y;
			z1 = weight[i] * coords1[i].z;

			g1 += x1 * coords1[i].x + y1 * coords1[i].y + z1 * coords1[i].z;

			x2 = coords2[i].x;
			y2 = coords2[i].y;
			z2 = coords2[i].z;

			g2 += weight[i] * (x2 * x2 + y2 * y2 + z2 * z2);

			Sxx += (x1 * x2);
			Sxy += (x1 * y2);
			Sxz += (x1 * z2);

			Syx += (y1 * x2);
			Syy += (y1 * y2);
			Syz += (y1 * z2);

			Szx += (z1 * x2);
			Szy += (z1 * y2);
			Szz += (z1 * z2);
		}
	} else {
		for (int i = 0; i < coords1.size(); i++) {
			g1 += coords1[i].x * coords1[i].x + coords1[i].y * coords1[i].y + coords1[i].z * coords1[i].z;
			g2 += coords2[i].x * coords2[i].x + coords2[i].y * coords2[i].y + coords2[i].z * coords2[i].z;

			Sxx += coords1[i].x * coords2[i].x;
			Sxy += coords1[i].x * coords2[i].y;
			Sxz += coords1[i].x * coords2[i].z;

			Syx += coords1[i].y * coords2[i].x;
			Syy += coords1[i].y * coords2[i].y;
			Syz += coords1[i].y * coords2[i].z;

			Szx += coords1[i].z * coords2[i].x;
			Szy += coords1[i].z * coords2[i].y;
			Szz += coords1[i].z * coords2[i].z;
		}
		// wsum = coords1.length;
	}

	e0 = (g1 + g2) * 0.5;

	SxzpSzx = Sxz + Szx;
	SyzpSzy = Syz + Szy;
	SxypSyx = Sxy + Syx;
	SyzmSzy = Syz - Szy;
	SxzmSzx = Sxz - Szx;
	SxymSyx = Sxy - Syx;
	SxxpSyy = Sxx + Syy;
	SxxmSyy = Sxx - Syy;
	mxEigenV = e0;

	innerProductCalculated = true;
}

void QCP::calcRmsd(PackedVector3Array &x, PackedVector3Array &y) {
	// QCP doesn't handle alignment of single values, so if we only have one point
	// we just compute regular distance.
	if (x.size() == 1) {
		rmsd = x[0].distance_to(y[0]);
		rmsdCalculated = true;
	} else {
		if (!innerProductCalculated) {
			innerProduct(y, x);
		}
		calcRmsd(wsum);
	}
}

Quaternion QCP::weightedSuperpose(PackedVector3Array &p_moved, PackedVector3Array &p_target, Vector<real_t> &p_weight, bool translate) {
	set(p_moved, p_target, p_weight, translate);
	return getRotation();
}

void QCP::set(PackedVector3Array &p_moved, PackedVector3Array &p_target, Vector<real_t> &p_weight, bool p_translate) {
	rmsdCalculated = false;
	transformationCalculated = false;
	innerProductCalculated = false;

	this->moved = p_moved;
	this->target = p_target;
	this->weight = p_weight;

	if (p_translate) {
		moveToWeightedCenter(this->moved, this->weight, movedCenter);
		wsum = 0; // set wsum to 0 so we don't double up.
		moveToWeightedCenter(this->target, this->weight, targetCenter);
		translate(movedCenter * -1, this->moved);
		translate(targetCenter * -1, this->target);
	} else {
		if (!p_weight.is_empty()) {
			for (int i = 0; i < p_weight.size(); i++) {
				wsum += p_weight[i];
			}
		} else {
			wsum = p_moved.size();
		}
	}
}