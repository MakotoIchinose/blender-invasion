/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 */

/** \file KX_MotionState.h
 *  \ingroup ketsji
 */

#ifndef __KX_MOTIONSTATE_H__
#define __KX_MOTIONSTATE_H__

#include "PHY_IMotionState.h"

#ifdef WITH_CXX_GUARDEDALLOC
#include "MEM_guardedalloc.h"
#endif

class KX_MotionState : public PHY_IMotionState
{
	class	SG_Spatial*		m_node;

public:
	KX_MotionState(class SG_Spatial* spatial);
	virtual ~KX_MotionState();

	virtual void	GetWorldPosition(float& posX,float& posY,float& posZ);
	virtual void	GetWorldScaling(float& scaleX,float& scaleY,float& scaleZ);
	virtual void	GetWorldOrientation(float& quatIma0,float& quatIma1,float& quatIma2,float& quatReal);
	virtual void	SetWorldPosition(float posX,float posY,float posZ);
	virtual	void	SetWorldOrientation(float quatIma0,float quatIma1,float quatIma2,float quatReal);
	virtual void	GetWorldOrientation(float* ori);
	virtual void	SetWorldOrientation(const float* ori);

	virtual	void	CalculateWorldTransformations();


#ifdef WITH_CXX_GUARDEDALLOC
	MEM_CXX_CLASS_ALLOC_FUNCS("GE:KX_MotionState")
#endif
};

#endif  /* __KX_MOTIONSTATE_H__ */