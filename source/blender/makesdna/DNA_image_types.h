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

/** \file
 * \ingroup DNA
 */

#ifndef __DNA_IMAGE_TYPES_H__
#define __DNA_IMAGE_TYPES_H__

#include "DNA_defs.h"
#include "DNA_ID.h"
#include "DNA_color_types.h"  /* for color management */

struct GPUTexture;
struct MovieCache;
struct PackedFile;
struct RenderResult;
struct Scene;
struct anim;

/* *************************************************************** */

typedef struct Stereo3dFormat {
	short flag;
	/** Encoding mode. */
	char display_mode;
	/** Anaglyph scheme for the user display. */
	char anaglyph_type;
	/** Interlace type for the user display. */
	char interlace_type;
	char pad[3];
} Stereo3dFormat;

/* Stereo3dFormat.display_mode */
typedef enum eStereoDisplayMode {
	S3D_DISPLAY_ANAGLYPH    = 0,
	S3D_DISPLAY_INTERLACE   = 1,
	S3D_DISPLAY_PAGEFLIP    = 2,
	S3D_DISPLAY_SIDEBYSIDE  = 3,
	S3D_DISPLAY_TOPBOTTOM   = 4,
} eStereoDisplayMode;

/* Stereo3dFormat.flag */
typedef enum eStereo3dFlag {
	S3D_INTERLACE_SWAP        = (1 << 0),
	S3D_SIDEBYSIDE_CROSSEYED  = (1 << 1),
	S3D_SQUEEZED_FRAME        = (1 << 2),
} eStereo3dFlag;

/* Stereo3dFormat.anaglyph_type */
typedef enum eStereo3dAnaglyphType {
	S3D_ANAGLYPH_REDCYAN      = 0,
	S3D_ANAGLYPH_GREENMAGENTA = 1,
	S3D_ANAGLYPH_YELLOWBLUE   = 2,
} eStereo3dAnaglyphType;

/* Stereo3dFormat.interlace_type */
typedef enum eStereo3dInterlaceType {
	S3D_INTERLACE_ROW          = 0,
	S3D_INTERLACE_COLUMN       = 1,
	S3D_INTERLACE_CHECKERBOARD = 2,
} eStereo3dInterlaceType;

/* *************************************************************** */

/* Generic image format settings,
 * this is used for NodeImageFile and IMAGE_OT_save_as operator too.
 *
 * note: its a bit strange that even though this is an image format struct
 * the imtype can still be used to select video formats.
 * RNA ensures these enum's are only selectable for render output.
 */
typedef struct ImageFormatData {
	/**
	 * R_IMF_IMTYPE_PNG, R_...
	 * \note, video types should only ever be set from this structure when used from RenderData.
	 */
	char imtype;
	/**
	 * bits per channel, R_IMF_CHAN_DEPTH_8 -> 32,
	 * not a flag, only set 1 at a time. */
	char depth;

	/** R_IMF_PLANES_BW, R_IMF_PLANES_RGB, R_IMF_PLANES_RGBA. */
	char planes;
	/** Generic options for all image types, alpha zbuffer. */
	char flag;

	/** (0 - 100), eg: jpeg quality. */
	char quality;
	/** (0 - 100), eg: png compression. */
	char compress;


	/* --- format specific --- */

	/* OpenEXR */
	char  exr_codec;

	/* Cineon */
	char  cineon_flag;
	short cineon_white, cineon_black;
	float cineon_gamma;

	/* Jpeg2000 */
	char  jp2_flag;
	char jp2_codec;

	/* TIFF */
	char tiff_codec;

	char pad[4];

	/* Multiview */
	char views_format;
	Stereo3dFormat stereo3d_format;

	/* color management */
	ColorManagedViewSettings view_settings;
	ColorManagedDisplaySettings display_settings;
} ImageFormatData;

/* ImageFormatData.imtype */
#define R_IMF_IMTYPE_TARGA           0
#define R_IMF_IMTYPE_IRIS            1
/* #define R_HAMX                    2 */ /* hamx is nomore */
/* #define R_FTYPE                   3 */ /* ftype is nomore */
#define R_IMF_IMTYPE_JPEG90          4
/* #define R_MOVIE                   5 */ /* movie is nomore */
#define R_IMF_IMTYPE_IRIZ            7
#define R_IMF_IMTYPE_RAWTGA         14
#define R_IMF_IMTYPE_AVIRAW         15
#define R_IMF_IMTYPE_AVIJPEG        16
#define R_IMF_IMTYPE_PNG            17
/* #define R_IMF_IMTYPE_AVICODEC    18 */ /* avicodec is nomore */
/* #define R_IMF_IMTYPE_QUICKTIME   19 */ /* quicktime is nomore */
#define R_IMF_IMTYPE_BMP            20
#define R_IMF_IMTYPE_RADHDR         21
#define R_IMF_IMTYPE_TIFF           22
#define R_IMF_IMTYPE_OPENEXR        23
#define R_IMF_IMTYPE_FFMPEG         24
/* #define R_IMF_IMTYPE_FRAMESERVER    25 */ /* frame server is nomore */
#define R_IMF_IMTYPE_CINEON         26
#define R_IMF_IMTYPE_DPX            27
#define R_IMF_IMTYPE_MULTILAYER     28
#define R_IMF_IMTYPE_DDS            29
#define R_IMF_IMTYPE_JP2            30
#define R_IMF_IMTYPE_H264           31
#define R_IMF_IMTYPE_XVID           32
#define R_IMF_IMTYPE_THEORA         33
#define R_IMF_IMTYPE_PSD            34

#define R_IMF_IMTYPE_INVALID        255

/* ImageFormatData.flag */
#define R_IMF_FLAG_ZBUF         (1<<0)   /* was R_OPENEXR_ZBUF */
#define R_IMF_FLAG_PREVIEW_JPG  (1<<1)   /* was R_PREVIEW_JPG */

/* return values from BKE_imtype_valid_depths, note this is depts per channel */
#define R_IMF_CHAN_DEPTH_1  (1<<0) /* 1bits  (unused) */
#define R_IMF_CHAN_DEPTH_8  (1<<1) /* 8bits  (default) */
#define R_IMF_CHAN_DEPTH_10 (1<<2) /* 10bits (uncommon, Cineon/DPX support) */
#define R_IMF_CHAN_DEPTH_12 (1<<3) /* 12bits (uncommon, jp2/DPX support) */
#define R_IMF_CHAN_DEPTH_16 (1<<4) /* 16bits (tiff, halff float exr) */
#define R_IMF_CHAN_DEPTH_24 (1<<5) /* 24bits (unused) */
#define R_IMF_CHAN_DEPTH_32 (1<<6) /* 32bits (full float exr) */

/* ImageFormatData.planes */
#define R_IMF_PLANES_RGB   24
#define R_IMF_PLANES_RGBA  32
#define R_IMF_PLANES_BW    8

/* ImageFormatData.exr_codec */
#define R_IMF_EXR_CODEC_NONE  0
#define R_IMF_EXR_CODEC_PXR24 1
#define R_IMF_EXR_CODEC_ZIP   2
#define R_IMF_EXR_CODEC_PIZ   3
#define R_IMF_EXR_CODEC_RLE   4
#define R_IMF_EXR_CODEC_ZIPS  5
#define R_IMF_EXR_CODEC_B44   6
#define R_IMF_EXR_CODEC_B44A  7
#define R_IMF_EXR_CODEC_DWAA  8
#define R_IMF_EXR_CODEC_DWAB  9
#define R_IMF_EXR_CODEC_MAX  10

/* ImageFormatData.jp2_flag */
#define R_IMF_JP2_FLAG_YCC          (1<<0)  /* when disabled use RGB */ /* was R_JPEG2K_YCC */
#define R_IMF_JP2_FLAG_CINE_PRESET  (1<<1)  /* was R_JPEG2K_CINE_PRESET */
#define R_IMF_JP2_FLAG_CINE_48      (1<<2)  /* was R_JPEG2K_CINE_48FPS */

/* ImageFormatData.jp2_codec */
#define R_IMF_JP2_CODEC_JP2  0
#define R_IMF_JP2_CODEC_J2K  1

/* ImageFormatData.cineon_flag */
#define R_IMF_CINEON_FLAG_LOG (1<<0)  /* was R_CINEON_LOG */

/* ImageFormatData.tiff_codec */
enum {
	R_IMF_TIFF_CODEC_DEFLATE   = 0,
	R_IMF_TIFF_CODEC_LZW       = 1,
	R_IMF_TIFF_CODEC_PACKBITS  = 2,
	R_IMF_TIFF_CODEC_NONE      = 3,
};

/* *************************************************************** */

/* ImageUser is in Texture, in Nodes, Background Image, Image Window, .... */
/* should be used in conjunction with an ID * to Image. */
typedef struct ImageUser {
	/** To retrieve render result. */
	struct Scene *scene;

	/** Movies, sequences: current to display. */
	int framenr;
	/** Total amount of frames to use. */
	int frames;
	/** Offset within movie, start frame in global time. */
	int offset, sfra;
	/** Cyclic flag. */
	char _pad, cycl;
	char ok;

	/** Multiview current eye - for internal use of drawing routines. */
	char multiview_eye;
	short pass;
	short pad;

	/** Listbase indices, for menu browsing or retrieve buffer. */
	short multi_index, view, layer;
	short flag;
} ImageUser;

typedef struct ImageAnim {
	struct ImageAnim *next, *prev;
	struct anim *anim;
} ImageAnim;

typedef struct ImageView {
	struct ImageView *next, *prev;
	/** MAX_NAME. */
	char name[64];
	/** 1024 = FILE_MAX. */
	char filepath[1024];
} ImageView;

typedef struct ImagePackedFile {
	struct ImagePackedFile *next, *prev;
	struct PackedFile *packedfile;
	/** 1024 = FILE_MAX. */
	char filepath[1024];
} ImagePackedFile;

typedef struct RenderSlot {
	struct RenderSlot *next, *prev;
	/** 64 = MAX_NAME. */
	char name[64];
	struct RenderResult *render;
} RenderSlot;

/* iuser->flag */
#define IMA_ANIM_ALWAYS         (1 << 0)
/* #define IMA_DEPRECATED_1     (1 << 1) */
/* #define IMA_DEPRECATED_2     (1 << 2) */
#define IMA_NEED_FRAME_RECALC   (1 << 3)
#define IMA_SHOW_STEREO         (1 << 4)

enum {
	TEXTARGET_TEXTURE_2D = 0,
	TEXTARGET_TEXTURE_CUBE_MAP = 1,
	TEXTARGET_COUNT = 2,
};

typedef struct Image {
	ID id;

	/** File path, 1024 = FILE_MAX. */
	char name[1024];

	/** Not written in file. */
	struct MovieCache *cache;
	/** Not written in file 2 = TEXTARGET_COUNT. */
	struct GPUTexture *gputexture[2];

	/* sources from: */
	ListBase anims;
	struct RenderResult *rr;

	ListBase renderslots;
	short render_slot, last_render_slot;

	int flag;
	short source, type;
	int lastframe;

	/* GPU texture flag. */
	short gpuflag;
	short pad2;
	unsigned int pad3;

	/** Deprecated. */
	struct PackedFile *packedfile DNA_DEPRECATED;
	struct ListBase packedfiles;
	struct PreviewImage *preview;

	int lastused;
	short ok;
	short pad4[3];

	/* for generated images */
	int gen_x, gen_y;
	char gen_type, gen_flag;
	short gen_depth;
	float gen_color[4];

	/* display aspect - for UV editing images resized for faster openGL display */
	float aspx, aspy;

	/* color management */
	ColorManagedColorspaceSettings colorspace_settings;
	char alpha_mode;

	char pad[5];

	/* Multiview */
	/** For viewer node stereoscopy. */
	char eye;
	char views_format;
	/** ImageView. */
	ListBase views;
	struct Stereo3dFormat *stereo3d_format;
} Image;


/* **************** IMAGE ********************* */

/* Image.flag */
enum {
	IMA_FLAG_DEPRECATED_0   = (1 << 0),  /* cleared */
	IMA_FLAG_DEPRECATED_1   = (1 << 1),  /* cleared */
#ifdef DNA_DEPRECATED
	IMA_DO_PREMUL           = (1 << 2),
#endif
	IMA_FLAG_DEPRECATED_4   = (1 << 4),  /* cleared */
	IMA_NOCOLLECT           = (1 << 5),
	IMA_FLAG_DEPRECATED_6   = (1 << 6),  /* cleared */
	IMA_OLD_PREMUL          = (1 << 7),
	IMA_FLAG_DEPRECATED_8   = (1 << 8),  /* cleared */
	IMA_USED_FOR_RENDER     = (1 << 9),
	/** For image user, but these flags are mixed. */
	IMA_USER_FRAME_IN_RANGE = (1 << 10),
	IMA_VIEW_AS_RENDER      = (1 << 11),
	IMA_IGNORE_ALPHA        = (1 << 12),
	IMA_DEINTERLACE         = (1 << 13),
	IMA_USE_VIEWS           = (1 << 14),
	IMA_FLAG_DEPRECATED_15  = (1 << 15),  /* cleared */
	IMA_FLAG_DEPRECATED_16  = (1 << 16),  /* cleared */
};

/* Image.gpuflag */
enum {
	/** GPU texture needs to be refreshed. */
	IMA_GPU_REFRESH =                 (1 << 0),
	/** All mipmap levels in OpenGL texture set? */
	IMA_GPU_MIPMAP_COMPLETE =         (1 << 1),
	/** OpenGL image texture bound as non-color data. */
	IMA_GPU_IS_DATA =                 (1 << 2),
};

/* ima->type and ima->source moved to BKE_image.h, for API */

/* render */
#define IMA_MAX_RENDER_TEXT		(1 << 9)

/* gen_flag */
#define IMA_GEN_FLOAT		1

/* alpha_mode */
enum {
	IMA_ALPHA_STRAIGHT = 0,
	IMA_ALPHA_PREMUL = 1,
};

#endif
