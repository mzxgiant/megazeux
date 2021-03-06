/* MegaZeux
 *
 * Copyright (C) 2007 Alan Williams <mralert@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdlib.h>

#include "platform.h"
#include "graphics.h"
#include "render.h"
#include "render_sdl.h"
#include "render_yuv.h"

bool yuv_set_video_mode_size(struct graphics_data *graphics,
 int width, int height, int depth, bool fullscreen, bool resize,
 int yuv_width, int yuv_height)
{
  struct yuv_render_data *render_data = graphics->render_data;

  // the YUV renderer _requires_ 32bit colour
  render_data->screen = SDL_SetVideoMode(width, height, 32,
   sdl_flags(depth, fullscreen, resize) | SDL_ANYFORMAT);

  if(!render_data->screen)
  {
    free(render_data);
    return false;
  }

  // try with a YUY2 pixel format first
  render_data->overlay = SDL_CreateYUVOverlay(yuv_width, yuv_height,
    SDL_YUY2_OVERLAY, render_data->screen);

  // didn't work, try with a UYVY pixel format next
  if(!render_data->overlay)
    render_data->overlay = SDL_CreateYUVOverlay(yuv_width, yuv_height,
      SDL_UYVY_OVERLAY, render_data->screen);

  // failed to create an overlay
  if(!render_data->overlay)
  {
    free(render_data);
    return false;
  }

  return true;
}

bool yuv_init_video(struct graphics_data *graphics, struct config_info *conf)
{
  struct yuv_render_data *render_data = cmalloc(sizeof(struct yuv_render_data));
  if(!render_data)
    return false;

  graphics->render_data = render_data;
  graphics->allow_resize = conf->allow_resize;
  render_data->ratio = conf->video_ratio;

  if(!set_video_mode())
  {
    free(render_data);
    return false;
  }

  return true;
}

void yuv_free_video(struct graphics_data *graphics)
{
  free(graphics->render_data);
  graphics->render_data = NULL;
}

bool yuv_check_video_mode(struct graphics_data *graphics,
 int width, int height, int depth, bool fullscreen, bool resize)
{
  // requires 32bit colour
  return SDL_VideoModeOK(width, height, 32,
   sdl_flags(depth, fullscreen, resize) | SDL_ANYFORMAT);
}

static inline void rgb_to_yuv(Uint8 r, Uint8 g, Uint8 b,
                              Uint8 *y, Uint8 *u, Uint8 *v)
{
  *y = (9797 * r + 19237 * g + 3734 * b) >> 15;
  *u = ((18492 * (b - *y)) >> 15) + 128;
  *v = ((23372 * (r - *y)) >> 15) + 128;
}

void yuv_update_colors(struct graphics_data *graphics,
 struct rgb_color *palette, Uint32 count)
{
  struct yuv_render_data *render_data = graphics->render_data;
  Uint8 y, u, v;
  Uint32 i;

  // it's a YUY2 overlay
  if(render_data->overlay->format == SDL_YUY2_OVERLAY)
  {
    for(i = 0; i < count; i++)
    {
      rgb_to_yuv(palette[i].r, palette[i].g, palette[i].b, &y, &u, &v);

#if PLATFORM_BYTE_ORDER == PLATFORM_BIG_ENDIAN
      graphics->flat_intensity_palette[i] =
        (v << 0) | (y << 8) | (u << 16) | (y << 24);
#else
      graphics->flat_intensity_palette[i] =
        (y << 0) | (u << 8) | (y << 16) | (v << 24);
#endif
    }
  }

  // it's a UYVY overlay
  else
  {
    for(i = 0; i < count; i++)
    {
      rgb_to_yuv(palette[i].r, palette[i].g, palette[i].b, &y, &u, &v);

#if PLATFORM_BYTE_ORDER == PLATFORM_BIG_ENDIAN
      graphics->flat_intensity_palette[i] =
        (y << 0) | (v << 8) | (y << 16) | (u << 24);
#else
      graphics->flat_intensity_palette[i] =
        (u << 0) | (y << 8) | (v << 16) | (y << 24);
#endif
    }
  }
}

void yuv_sync_screen(struct graphics_data *graphics)
{
  struct yuv_render_data *render_data = graphics->render_data;
  int width = render_data->screen->w, v_width;
  int height = render_data->screen->h, v_height;
  SDL_Rect rect;

  // FIXME: Putting this here is suboptimal
  fix_viewport_ratio(width, height, &v_width, &v_height, render_data->ratio);

  rect.x = (width - v_width) >> 1;
  rect.y = (height - v_height) >> 1;
  rect.w = v_width;
  rect.h = v_height;
  SDL_DisplayYUVOverlay(render_data->overlay, &rect);
}
