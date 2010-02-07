/* MegaZeux
 *
 * Copyright (C) 1996 Greg Janson
 * Copyright (C) 1999 Charles Goetzman
 * Copyright (C) 2004 Gilead Kutnick <exophase@adelphia.net>
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


// Main title screen/gaming code

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "event.h"
#include "helpsys.h"
#include "scrdisp.h"
#include "error.h"
#include "idarray.h"
#include "audio.h"
#include "edit.h"
#include "sfx.h"
#include "hexchar.h"
#include "idput.h"
#include "data.h"
#include "const.h"
#include "game.h"
#include "window.h"
#include "graphics.h"
#include "counter.h"
#include "game2.h"
#include "sprite.h"
#include "world.h"
#include "delay.h"
#include "robot.h"
#include "fsafeopen.h"

static char main_menu[] =
 "Enter- Menu\n"
 "Esc  - Exit to DOS\n"
 "F1/H - Help\n"
 "F2/S - Settings\n"
 "F3/L - Load world\n"
 "F4/R - Restore game\n"
 "F5/P - Play world\n"
 "F8/E - World editor\n"
 "F10  - Quickload\n"
 ""  // unused
 ""; // unused

static char game_menu[] =
 "F1    - Help\n"
 "Enter - Menu/status\n"
 "Esc   - Exit to title\n"
 "F2    - Settings\n"
 "F3    - Save game\n"
 "F4    - Restore game\n"
 "F5/Ins- Toggle bomb type\n"
 "\n"  // Debug Menu (disabled in game)
 "F9    - Quicksave\n"
 "F10   - Quickload\n"
 "Arrows- Move\n"
 "Space - Shoot (w/dir)\n"
 "Delete- Bomb";

static char editing_menu[] =
 "F1    - Help\n"
 "Enter - Menu/status\n"
 "Esc   - Exit to title\n"
 "F2    - Settings\n"
 "F3    - Save game\n"
 "F4    - Restore game\n"
 "F5/Ins- Toggle bomb type\n"
 "F6    - Debug Menu\n"
 "F9    - Quicksave\n"
 "F10   - Quickload\n"
 "Arrows- Move\n"
 "Space - Shoot (w/dir)\n"
 "Delete- Bomb";

char *world_ext[] = { ".MZX", NULL };

char *save_ext[] = { ".SAV", NULL };

char debug_mode = 0;

int update_music;

// For changing screens AFTER an update is done and shown
// -1 for teleport (so fading isn't used)
// For RESTORE/EXCHANGE PLAYER POSITION with DUPLICATION.
int target_d_id = -1;
// For RESTORE/EXCHANGE PLAYER POSITION with DUPLICATION.
int target_d_color = -1;

int pal_update; // Whether to update a palette from robot activity

void load_world_selection(World *mzx_world)
{
  char world_name[512];

  m_show();
  if(!choose_file_ch(mzx_world, world_ext,
   world_name, "Load World", 1))
  {
    strcpy(curr_file, world_name);
    load_world_file(mzx_world, curr_file);
  }
}

void load_world_file(World *mzx_world, char *name)
{
  Board *src_board;
  int fade = 0;

  // Load world curr_file
  end_mod();
  clear_sfx_queue();
  //Clear screen
  clear_screen(32, 7);
  // Palette
  default_palette();
  reload_world(mzx_world, name, &fade);
  send_robot_def(mzx_world, 0, 10);

  src_board = mzx_world->current_board;
  load_mod(src_board->mod_playing);
  strcpy(mzx_world->real_mod_playing, src_board->mod_playing);
  set_counter(mzx_world, "TIME", src_board->time_limit, 0);

  set_mesg(mzx_world,
   "** BETA **    F1: Help   Enter: Menu   Ctrl-Alt-Enter: Fullscreen");
}

void title_screen(World *mzx_world)
{
  int fadein = 1;
  int key = 0;
  int fade;
  struct stat file_info;
  Board *src_board;
  char current_dir[MAX_PATH];

  debug_mode = 0;

  // Clear screen
  clear_screen(32, 7);
  default_palette();

  getcwd(current_dir, MAX_PATH);

  chdir(config_dir);
  set_config_from_file(&(mzx_world->conf), "title.cnf");
  chdir(current_dir);

  // Try to load curr_file
  if(!stat(curr_file, &file_info))
  {
    load_world_file(mzx_world, curr_file);
  }
  else
  {
    load_world_selection(mzx_world);
  }

  src_board = mzx_world->current_board;

  // Main game loop
  // Mouse remains hidden unless menu/etc. is invoked

  update_screen();

  do
  {
    // Update
    if(mzx_world->active)
    {
      if(update(mzx_world, 0, &fadein))
      {
        update_event_status();
        continue;
      }
    }
    else
    {
      // Give some delay time if nothing's loaded
      update_event_status_delay();
    }

    src_board = mzx_world->current_board;

    update_event_status();

    // Keycheck
    key = get_key(keycode_SDL);

    if(key)
    {
      switch(key)
      {
        case SDLK_e: // E
        case SDLK_F8: // F8
        {
          // Editor
          clear_sfx_queue();
          vquick_fadeout();
          edit_world(mzx_world);

          if(curr_file[0])
            load_world_file(mzx_world, curr_file);

          fadein = 1;
          break;
        }

        case SDLK_s: // S
        case SDLK_F2: // F2
        {
          // Settings
          m_show();

          game_settings(mzx_world);

          update_screen();
          update_event_status();
          break;
        }

        case SDLK_KP_ENTER:
        case SDLK_RETURN: // Enter
        {
          if(get_counter(mzx_world, "ENTER_MENU", 0))
          {
            int key;

            save_screen();
            draw_window_box(30, 4, 52, 16, 25, 16, 24, 1, 1);
            write_string(main_menu, 32, 5, 31, 1);
            write_string(" Main Menu ", 36, 4, 30, 0);
            update_screen();

            m_show();

            do
            {
              update_event_status_delay();
              update_screen();
              key = get_key(keycode_SDL);
            } while((key != SDLK_RETURN) &&
             (key != SDLK_KP_ENTER));

            restore_screen();
            update_screen();
            update_event_status();
          }
          break;
        }

        case SDLK_ESCAPE: // ESC
        {
          // Quit
          m_show();

          if(confirm(mzx_world, "Exit MegaZeux - Are you sure?"))
            key = 0;

          update_screen();
          update_event_status();
          break;
        }

        case SDLK_l: // L
        case SDLK_F3: // F3
        {
          load_world_selection(mzx_world);
          fadein = 1;
          src_board = mzx_world->current_board;
          update_screen();
          update_event_status();
          break;
        }

        case SDLK_r: // R
        case SDLK_F4: // F4
        {
          char save_file_name[64];

          // Restore
          m_show();

          if(!choose_file_ch(mzx_world, save_ext, save_file_name,
           "Choose game to restore", 1))
          {
            // Swap out current board...
            clear_sfx_queue();
            // Load game
            fadein = 0;

            getcwd(current_dir, MAX_PATH);

            chdir(config_dir);
            set_config_from_file(&(mzx_world->conf), "game.cnf");
            chdir(current_dir);

            if(reload_savegame(mzx_world, save_file_name, &fadein))
            {
              vquick_fadeout();
            }
            else
            {
              src_board = mzx_world->current_board;
              // Swap in starting board
              load_mod(src_board->mod_playing);
              strcpy(mzx_world->real_mod_playing,
               src_board->mod_playing);

              send_robot_def(mzx_world, 0, 10);
              // Copy filename
              strcpy(curr_sav, save_file_name);
              fadein ^= 1;

              send_robot_def(mzx_world, 0, 11);
              send_robot_def(mzx_world, 0, 10);
              set_counter(mzx_world, "TIME", src_board->time_limit, 0);

              find_player(mzx_world);
              mzx_world->player_restart_x = mzx_world->player_x;
              mzx_world->player_restart_y = mzx_world->player_y;
              vquick_fadeout();

              play_game(mzx_world, 1);

              // Done playing- load world again
              // Already faded out from play_game()
              end_mod();
              // Clear screen
              clear_screen(32, 7);
              // Palette
              default_palette();
              insta_fadein();
              // Reload original file
              if(!stat(curr_file, &file_info))
              {
                reload_world(mzx_world, curr_file, &fade);

                src_board = mzx_world->current_board;
                load_mod(src_board->mod_playing);
                strcpy(mzx_world->real_mod_playing,
                 src_board->mod_playing);
                set_counter(mzx_world, "TIME",
                 src_board->time_limit, 0);
              }
              else
              {
                clear_world(mzx_world);
              }
              vquick_fadeout();
              fadein = 1;
            }
            break;
          }

          update_screen();
          update_event_status();
          break;
        }

        case SDLK_p: // P
        case SDLK_F5: // F5
        {
          if(mzx_world->active)
          {
            char old_mod_playing[128];
            strcpy(old_mod_playing, mzx_world->real_mod_playing);

            // Play
            // Only from swap?

            if(mzx_world->only_from_swap)
            {
              m_show();
              error("You can only play this game via a swap"
               " from another game", 0, 24, 0x3101);
              break;
            }

            // Load world curr_file
            // Don't end mod- We want a smooth transition for that.
            // Clear screen

            clear_screen(32, 7);
            // Palette
            default_palette();

            getcwd(current_dir, MAX_PATH);

            chdir(config_dir);
            set_config_from_file(&(mzx_world->conf), "game.cnf");
            chdir(current_dir);

            reload_world(mzx_world, curr_file, &fade);

            mzx_world->current_board_id = mzx_world->first_board;
            mzx_world->current_board =
             mzx_world->board_list[mzx_world->current_board_id];
            src_board = mzx_world->current_board;

            send_robot_def(mzx_world, 0, 11);
            send_robot_def(mzx_world, 0, 10);

            if(strcmp(src_board->mod_playing, "*") &&
             strcmp(src_board->mod_playing, old_mod_playing))
              load_mod(src_board->mod_playing);

            strcpy(mzx_world->real_mod_playing,
             src_board->mod_playing);

            set_counter(mzx_world, "TIME", src_board->time_limit, 0);

            clear_sfx_queue();
            find_player(mzx_world);
            mzx_world->player_restart_x = mzx_world->player_x;
            mzx_world->player_restart_y = mzx_world->player_y;
            vquick_fadeout();

            play_game(mzx_world, 1);
            // Done playing- load world again
            // Already faded out from play_game()
            end_mod();
            // Clear screen
            clear_screen(32, 7);
            // Palette
            default_palette();
            insta_fadein();
            // Reload original file
            reload_world(mzx_world, curr_file, &fade);
            src_board = mzx_world->current_board;
            load_mod(src_board->mod_playing);
            strcpy(mzx_world->real_mod_playing,
             src_board->mod_playing);
            set_counter(mzx_world, "TIME", src_board->time_limit, 0);
            vquick_fadeout();
            fadein = 1;
          }
          break;
        }

        case SDLK_F1:
        {
          if(get_counter(mzx_world, "HELP_MENU", 0) ||
            (!mzx_world->active))
          {
            m_show();
            help_system(mzx_world);
            update_screen();
          }
          break;
        }

        // Quick load
        case SDLK_F10:
        {
          // Restore
          m_show();

          // Swap out current board...
          clear_sfx_queue();
          // Load game
          fadein = 0;

          getcwd(current_dir, MAX_PATH);

          chdir(config_dir);
          set_config_from_file(&(mzx_world->conf), "game.cnf");
          chdir(current_dir);

          if(reload_savegame(mzx_world, curr_sav, &fadein))
          {
            vquick_fadeout();
          }
          else
          {
            src_board = mzx_world->current_board;
            // Swap in starting board
            load_mod(src_board->mod_playing);
            strcpy(mzx_world->real_mod_playing,
             src_board->mod_playing);

            send_robot_def(mzx_world, 0, 10);
            fadein ^= 1;

            send_robot_def(mzx_world, 0, 11);
            send_robot_def(mzx_world, 0, 10);
            set_counter(mzx_world, "TIME", src_board->time_limit, 0);

            find_player(mzx_world);
            mzx_world->player_restart_x = mzx_world->player_x;
            mzx_world->player_restart_y = mzx_world->player_y;
            vquick_fadeout();

            play_game(mzx_world, 1);

            // Done playing- load world again
            // Already faded out from play_game()
            end_mod();
            // Clear screen
            clear_screen(32, 7);
            // Palette
            default_palette();
            insta_fadein();
            // Reload original file
            if(!stat(curr_file, &file_info))
            {
              reload_world(mzx_world, curr_file, &fade);
              src_board = mzx_world->current_board;
              load_mod(src_board->mod_playing);
              strcpy(mzx_world->real_mod_playing,
               src_board->mod_playing);
              set_counter(mzx_world, "TIME",
               src_board->time_limit, 0);
            }
            else
            {
              clear_world(mzx_world);
            }
            vquick_fadeout();
            fadein = 1;
          }

          update_screen();
          update_event_status();

          break;
        }
      }
    }
  } while(key != SDLK_ESCAPE);

  vquick_fadeout();
  clear_sfx_queue();
}

void draw_viewport(World *mzx_world)
{
  int i, i2;
  Board *src_board = mzx_world->current_board;
  int viewport_x = src_board->viewport_x;
  int viewport_y = src_board->viewport_y;
  int viewport_width = src_board->viewport_width;
  int viewport_height = src_board->viewport_height;
  char edge_color = mzx_world->edge_color;

  // Draw the current viewport
  if(viewport_y > 1)
  {
    // Top
    for(i = 0; i < viewport_y; i++)
      fill_line_ext(80, 0, i, 177, edge_color, 0, 0);
  }

  if((viewport_y + viewport_height) < 24)
  {
    // Bottom
    for(i = viewport_y + viewport_height + 1; i < 25; i++)
      fill_line_ext(80, 0, i, 177, edge_color, 0, 0);
  }

  if(viewport_x > 1)
  {
    // Left
    for(i = 0; i < 25; i++)
      fill_line_ext(viewport_x, 0, i, 177, edge_color, 0, 0);
  }

  if((viewport_x + viewport_width) < 79)
  {
    // Right
    i2 = viewport_x + viewport_width + 1;
    for(i = 0; i < 25; i++)
    {
      fill_line_ext(80 - i2, i2, i, 177, edge_color, 0, 0);
    }
  }

  // Draw the box
  if(viewport_x > 0)
  {
    // left
    for(i = 0; i < viewport_height; i++)
    {
      draw_char_ext('\xba', edge_color, viewport_x - 1,
       i + viewport_y, 0, 0);
    }

    if(viewport_y > 0)
    {
      draw_char_ext('\xc9', edge_color, viewport_x - 1,
       viewport_y - 1, 0, 0);
    }
  }
  if((viewport_x + viewport_width) < 80)
  {
    // right
    for(i = 0; i < viewport_height; i++)
    {
      draw_char_ext('\xba', edge_color,
       viewport_x + viewport_width, i + viewport_y, 0, 0);
    }

    if(viewport_y > 0)
    {
      draw_char_ext('\xbb', edge_color,
       viewport_x + viewport_width, viewport_y - 1, 0, 0);
    }
  }

  if(viewport_y > 0)
  {
    // top
    for(i = 0; i < viewport_width; i++)
    {
      draw_char_ext('\xcd', edge_color, viewport_x + i,
       viewport_y - 1, 0, 0);
    }
  }

  if((viewport_y + viewport_height) < 25)
  {
    // bottom
    for(i = 0; i < viewport_width; i++)
    {
      draw_char_ext('\xcd', edge_color, viewport_x + i,
       viewport_y + viewport_height, 0, 0);
    }

    if(viewport_x > 0)
    {
      draw_char_ext('\xc8', edge_color, viewport_x - 1,
       viewport_y + viewport_height, 0, 0);
    }

    if((viewport_x + viewport_width) < 80)
    {
      draw_char_ext('\xbc', edge_color, viewport_x + viewport_width,
       viewport_y + viewport_height, 0, 0);
    }
  }
}

// Updates game variables
// Slowed = 1 to not update lazwall or time
// due to slowtime or freezetime

void update_variables(World *mzx_world, int slowed)
{
  Board *src_board = mzx_world->current_board;
  int blind_dur = mzx_world->blind_dur;
  int firewalker_dur = mzx_world->firewalker_dur;
  int freeze_time_dur = mzx_world->freeze_time_dur;
  int slow_time_dur = mzx_world->slow_time_dur;
  int wind_dur = mzx_world->wind_dur;
  int b_mesg_timer = src_board->b_mesg_timer;
  int invinco;
  int lazwall_start = src_board->lazwall_start;
  static int slowdown = 0;
  slowdown ^= 1;

  // If odd, we...
  if(!slowdown)
  {
    // Change scroll color
    scroll_color++;

    if(scroll_color > 15)
      scroll_color = 7;

    // Decrease time limit
    if(!slowed)
    {
      int time_left = get_counter(mzx_world, "TIME", 0);
      if(time_left > 0)
      {
        if(time_left == 1)
        {
          // Out of time
          dec_counter(mzx_world, "HEALTH", 10, 0);
          set_mesg(mzx_world, "Out of time!");
          play_sfx(mzx_world, 42);
          // Reset time
          set_counter(mzx_world, "TIME", src_board->time_limit, 0);
        }
        else
        {
          dec_counter(mzx_world, "TIME", 1, 0);
        }
      }
    }
  }
  // Decrease effect durations
  if(blind_dur > 0)
    mzx_world->blind_dur = blind_dur - 1;

  if(firewalker_dur > 0)
    mzx_world->firewalker_dur = firewalker_dur - 1;

  if(freeze_time_dur > 0)
    mzx_world->freeze_time_dur = freeze_time_dur - 1;

  if(slow_time_dur > 0)
    mzx_world->slow_time_dur = slow_time_dur - 1;

  if(wind_dur > 0)
    mzx_world->wind_dur = wind_dur - 1;

  // Decrease message timer
  if(b_mesg_timer > 0)
    src_board->b_mesg_timer = b_mesg_timer - 1;

  // Invinco
  invinco = get_counter(mzx_world, "INVINCO", 0);
  if(invinco > 0)
  {
    if(invinco == 1)
    {
      // Just finished-
      set_counter(mzx_world, "INVINCO", 0, 0);
    }
    else
    {
      // Decrease
      dec_counter(mzx_world, "INVINCO", 1, 0);
      play_sfx(mzx_world, 17);
      id_chars[player_color] = rand() & 255;
    }
  }
  // Lazerwall start- cycle 0 to 7 then -7 to -1
  if(!slowed)
  {
    if(((signed char)lazwall_start) < 7)
      src_board->lazwall_start = lazwall_start + 1;
    else
      src_board->lazwall_start = -7;
  }
  // Done
}

void set_mesg(World *mzx_world, char *str)
{
  if(mzx_world->bi_mesg_status)
  {
    set_mesg_direct(mzx_world->current_board, str);
  }
}

void set_mesg_direct(Board *src_board, char *str)
{
  char *bottom_mesg = src_board->bottom_mesg;

  // Sets the current message
  if(strlen(str) > 80)
  {
    memcpy(bottom_mesg, str, 80);
    bottom_mesg[80] = 0;
  }
  else
  {
    strcpy(bottom_mesg, str);
  }
  src_board->b_mesg_timer = 160;
}

void set_3_mesg(World *mzx_world, char *str1, int num, char *str2)
{
  if(mzx_world->bi_mesg_status)
  {
    Board *src_board = mzx_world->current_board;
    sprintf(src_board->bottom_mesg, "%s%d%s", str1, num, str2);
    src_board->b_mesg_timer = 160;
  }
}

//Bit 1- +1
//Bit 2- -1
//Bit 4- +width
//Bit 8- -width

char cw_offs[8] = { 10, 2, 6, 4, 5, 1, 9, 8 };
char ccw_offs[8] = { 10, 8, 9, 1, 5, 4, 6, 2 };

// Rotate an area
void rotate(World *mzx_world, int x, int y, int dir)
{
  Board *src_board = mzx_world->current_board;
  char *offsp = cw_offs;
  int offs[8];
  int offset, i;
  int board_width = src_board->board_width;
  int board_height = src_board->board_height;
  int cw, ccw;
  char *level_id = src_board->level_id;
  char *level_param = src_board->level_param;
  char *level_color = src_board->level_color;
  mzx_thing id;
  char param, color;
  mzx_thing cur_id;
  int d_flag;
  int cur_offset, next_offset;

  offset = x + (y * board_width);
  if((x == 0) || (y == 0) || (x == (board_width - 1)) ||
   (y == (board_height - 1))) return;

  if(dir)
    offsp = ccw_offs;

  // Fix offsets
  for(i = 0; i < 8; i++)
  {
    int offsval = offsp[i];
    if(offsval & 1)
      offs[i] = 1;
    else
      offs[i] = 0;

    if(offsval & 2)
      offs[i]--;

    if(offsval & 4)
      offs[i] += board_width;

    if(offsval & 8)
      offs[i] -= board_width;
  }

  for(i = 0; i < 8; i++)
  {
    cur_id = (mzx_thing)level_id[offset + offs[i]];
    if((flags[(int)cur_id] & A_UNDER) && (cur_id != GOOP))
      break;
  }

  if(i == 8)
  {
    for(i = 0; i < 8; i++)
    {
      cur_id = (mzx_thing)level_id[offset + offs[i]];
      d_flag = flags[(int)cur_id];

      if((!(d_flag & A_PUSHABLE) || (d_flag & A_SPEC_PUSH)) &&
       (cur_id != GATE))
      {
        break; // Transport NOT pushable
      }
    }

    if(i == 8)
    {
      cur_offset = offset + offs[0];
      id = (mzx_thing)level_id[cur_offset];
      color = level_color[cur_offset];
      param = level_param[cur_offset];

      for(i = 0; i < 7; i++)
      {
        cur_offset = offset + offs[i];
        next_offset = offset + offs[i + 1];
        level_id[cur_offset] = level_id[next_offset];
        level_color[cur_offset] = level_color[next_offset];
        level_param[cur_offset] = level_param[next_offset];
      }

      cur_offset = offset + offs[7];
      level_id[cur_offset] = (char)id;
      level_color[cur_offset] = color;
      level_param[cur_offset] = param;
    }
  }
  else
  {
    cw = i - 1;

    if(cw == -1)
      cw = 7;

    do
    {
      ccw = i + 1;
      if(ccw == 8)
        ccw = 0;

      cur_offset = offset + offs[ccw];
      next_offset = offset + offs[i];
      cur_id = (mzx_thing)level_id[cur_offset];
      d_flag = flags[(int)cur_id];

      if(((d_flag & A_PUSHABLE) || (d_flag & A_SPEC_PUSH)) &&
       (cur_id != GATE) && (!(mzx_world->update_done[cur_offset] & 2)))
      {
        offs_place_id(mzx_world, next_offset, cur_id,
         level_color[cur_offset], level_param[cur_offset]);
        offs_remove_id(mzx_world, cur_offset);
        mzx_world->update_done[offset + offs[i]] |= 2;
        i = ccw;
      }
      else
      {
        i = ccw;
        while(i != cw)
        {
          cur_id = (mzx_thing)level_id[offset + offs[i]];
          if((flags[(int)cur_id] & A_UNDER) && (cur_id != GOOP))
            break;

          i++;
          if(i == 8)
            i = 0;
        }
      }
    } while(i != cw);
  }
}

void calculate_xytop(World *mzx_world, int *x, int *y)
{
  Board *src_board = mzx_world->current_board;
  int nx, ny;
  int board_width = src_board->board_width;
  int board_height = src_board->board_height;
  int viewport_width = src_board->viewport_width;
  int viewport_height = src_board->viewport_height;
  int locked_y = src_board->locked_y;

  // Calculate xy top from player position and scroll view pos, or
  // as static position if set.
  if(locked_y != -1)
  {
    nx = src_board->locked_x + src_board->scroll_x;
    ny = locked_y + src_board->scroll_y;
  }
  else
  {
    // Calculate from player position
    // Center screen around player, add scroll factor
    nx = mzx_world->player_x - (viewport_width / 2);
    ny = mzx_world->player_y - (viewport_height / 2);

    if(nx < 0)
      nx = 0;

    if(ny < 0)
      ny = 0;

    if(nx > (board_width - viewport_width))
     nx = board_width - viewport_width;

    if(ny > (board_height - viewport_height))
     ny = board_height - viewport_height;

    nx += src_board->scroll_x;
    ny += src_board->scroll_y;
  }
  // Prevent from going offscreen
  if(nx < 0)
    nx = 0;

  if(ny < 0)
    ny = 0;

  if(nx > (board_width - viewport_width))
    nx = board_width - viewport_width;

  if(ny > (board_height - viewport_height))
    ny = board_height - viewport_height;

  *x = nx;
  *y = ny;
}

void update_player(World *mzx_world)
{
  Board *src_board = mzx_world->current_board;
  int player_x = mzx_world->player_x;
  int player_y = mzx_world->player_y;
  int board_width = src_board->board_width;
  mzx_thing under_id =
   (mzx_thing)src_board->level_under_id[player_x +
   (player_y * board_width)];

  // t1 = ID stood on
  if(!(flags[under_id] & A_AFFECT_IF_STOOD))
  {
    return; // Nothing special
  }

  switch(under_id)
  {
    case ICE:
    {
      int player_last_dir = src_board->player_last_dir;
      if(player_last_dir & 0x0F)
      {
        if(move_player(mzx_world, (player_last_dir & 0x0F) - 1))
          src_board->player_last_dir = player_last_dir & 0xF0;
      }
      break;
    }

    case LAVA:
    {
      if(mzx_world->firewalker_dur > 0)
        break;

      play_sfx(mzx_world, 22);
      set_mesg(mzx_world, "Augh!");
      dec_counter(mzx_world, "HEALTH", id_dmg[26], 0);
      return;
    }

    case FIRE:
    {
      if(mzx_world->firewalker_dur > 0)
        break;

      play_sfx(mzx_world, 43);
      set_mesg(mzx_world, "Ouch!");
      dec_counter(mzx_world, "HEALTH", id_dmg[63], 0);
      return;
    }

    default:
    {
      move_player(mzx_world, under_id - 21);
      break;
    }
  }

  find_player(mzx_world);
}

// Settings dialog-

//----------------------------
//
//  Speed- 123456789
//
//   ( ) Digitized music on
//   ( ) Digitized music off
//
//   ( ) PC speaker SFX on
//   ( ) PC speaker SFX off
//
//  Sound card volumes-
//  Overall volume- 12345678
//  SoundFX volume- 12345678
//
//    OK        Cancel
//
//----------------------------


void game_settings(World *mzx_world)
{
  Board *src_board = mzx_world->current_board;
  int mzx_speed, music, sfx;
  int music_volume, sound_volume, sfx_volume;
  dialog di;
  int dialog_result;
  int speed_option = 0;
  int num_elements = 8;
  int start_option = 0;

  char *radio_strings_1[2] =
  {
    "Digitized music on", "Digitized music off"
  };
  char *radio_strings_2[2] =
  {
    "PC speaker SFX on", "PC speaker SFX off"
  };
  element *elements[9];

  if(!mzx_world->lock_speed)
  {
    speed_option = 2;
    num_elements = 9;
    start_option = 8;
    elements[8] = construct_number_box(5, 2, "Speed- ", 1, 9,
     0, &mzx_speed);
  }

  elements[0] = construct_radio_button(4, 2 + speed_option,
   radio_strings_1, 2, 19, &music);
  elements[1] = construct_radio_button(4, 5 + speed_option,
   radio_strings_2, 2, 18, &sfx);
  elements[2] = construct_label(3, 8 + speed_option,
   "Audio volumes-");
  elements[3] = construct_number_box(3, 9 + speed_option,
   "Overall volume- ", 1, 8, 0, &music_volume);
  elements[4] = construct_number_box(3, 10 + speed_option,
   "SoundFX volume- ", 1, 8, 0, &sound_volume);
  elements[5] = construct_number_box(3, 11 + speed_option,
   "PC Speaker SFX- ", 1, 8, 0, &sfx_volume);
  elements[6] = construct_button(7, 13 + speed_option, "OK", 0);
  elements[7] = construct_button(16, 13 + speed_option,
   "Cancel", 1);

  construct_dialog(&di, "Game Settings", 25, 4 - (speed_option / 2),
   30, 16 + speed_option, elements, num_elements, start_option);

  set_context(92);
  mzx_speed = mzx_world->mzx_speed;
  music = get_music_on_state() ^ 1;
  sfx = get_sfx_on_state() ^ 1;
  music_volume = get_music_volume();
  sound_volume = get_sound_volume();
  sfx_volume = get_sfx_volume();

  dialog_result = run_dialog(mzx_world, &di);
  destruct_dialog(&di);
  pop_context();

  if(!dialog_result)
  {
    set_sfx_volume(sfx_volume);

    if(music_volume != get_music_volume())
    {
      set_music_volume(music_volume);

      if(mzx_world->active)
        volume_mod(src_board->volume);
    }

    if(sound_volume != get_sound_volume())
      set_sound_volume(sound_volume);

    sfx ^= 1;
    music ^= 1;

    set_sfx_on(sfx);

    if(!music)
    {
      // Turn off music.
      if(get_music_on_state() && (mzx_world->active))
        end_mod();
    }
    else

    if(!get_music_on_state() && (mzx_world->active))
    {
      // Turn on music.
      strcpy(mzx_world->real_mod_playing, src_board->mod_playing);
      load_mod(mzx_world->real_mod_playing);
    }

    set_music_on(music);
    mzx_world->mzx_speed = mzx_speed;
  }
}

// Number of cycles to make player idle before repeating a
// directional move

#define REPEAT_WAIT 2

void play_game(World *mzx_world, int fadein)
{
  // We have the world loaded, on the proper scene.
  // We are faded out. Commence playing!
  int key = -1;
  char keylbl[5] = "KEY?";
  Board *src_board;

  // Main game loop
  // Mouse remains hidden unless menu/etc. is invoked

  set_context(91);

  do
  {
    // Update
    if(update(mzx_world, 1, &fadein))
    {
      update_event_status();
      continue;
    }
    update_event_status();

    src_board = mzx_world->current_board;

    // Keycheck

    key = get_key(keycode_SDL);

    if(key)
    {
      int key_char = get_key(keycode_unicode);

      if(key_char)
      {
        keylbl[3] = key_char;
        send_robot_all(mzx_world, keylbl);
      }

      switch(key)
      {
        case SDLK_F1:
        {
          if(get_counter(mzx_world, "HELP_MENU", 0))
          {
            m_show();
            help_system(mzx_world);
          }
          break;
        }

        case SDLK_F2:
        {
          // Settings
          if(get_counter(mzx_world, "F2_MENU", 0) ||
           (!mzx_world->active))
          {
            m_show();

            game_settings(mzx_world);

            update_event_status();
          }
          break;
        }

        case SDLK_KP_ENTER:
        case SDLK_RETURN:
        {
          int enter_menu_status =
           get_counter(mzx_world, "ENTER_MENU", 0);
          send_robot_all(mzx_world, "KeyEnter");
          // Menu
          // 19x9
          if(enter_menu_status)
          {
            int key;
            save_screen();

            draw_window_box(8, 4, 35, 18, 25, 16, 24, 1, 1);
            if (mzx_world->editing)
              write_string(editing_menu, 10, 5, 31, 1);
            else
              write_string(game_menu, 10, 5, 31, 1);
            write_string(" Game Menu ", 14, 4, 30, 0);
            show_status(mzx_world); // Status screen too
            update_screen();
            m_show();
            do
            {
              update_event_status_delay();
              update_screen();
              key = get_key(keycode_SDL);
            } while((key != SDLK_RETURN) &&
             (key != SDLK_KP_ENTER));

            restore_screen();

            update_event_status();
          }
          break;
        }

        case SDLK_ESCAPE:
        {
          // Quit
          m_show();

          if(confirm(mzx_world, "Quit playing- Are you sure?"))
            key = 0;

          update_event_status();
          break;
        }

        case SDLK_F3:
        {
          // Save game
          if(!mzx_world->dead)
          {
            // Can we?
            if((src_board->save_mode != CANT_SAVE) &&
             ((src_board->save_mode != CAN_SAVE_ON_SENSOR) ||
             (src_board->level_under_id[mzx_world->player_x +
             (src_board->board_width * mzx_world->player_y)] ==
             SENSOR)))
            {
              char save_game[512];

              strcpy(save_game, curr_sav);

              m_show();
              if(!new_file(mzx_world, save_ext, save_game,
               "Save game", 1))
              {
                strcpy(curr_sav, save_game);
                // Name in curr_sav....
                add_ext(curr_sav, ".sav");
                // Save entire game
                save_world(mzx_world, curr_sav, 1, get_fade_status());
              }
            }
          }
          update_event_status();
          break;
        }

        case SDLK_F4:
        {
          // Restore
          char save_file_name[64];
          m_show();

          if(!choose_file_ch(mzx_world, save_ext, save_file_name,
           "Choose game to restore", 1))
          {
            // Load game
            fadein = 0;
            if(reload_savegame(mzx_world, save_file_name, &fadein))
            {
              vquick_fadeout();
              return;
            }

            // Reset this
            src_board = mzx_world->current_board;
            // Swap in starting board
            load_mod(src_board->mod_playing);
            strcpy(mzx_world->real_mod_playing,
             src_board->mod_playing);

            find_player(mzx_world);

            strcpy(curr_sav, save_file_name);
            send_robot_def(mzx_world, 0, 10);
            fadein ^= 1;
          }

          update_event_status();
          break;
        }

        case SDLK_F5:
        case SDLK_INSERT:
        {
          // Change bomb type
          if(!mzx_world->dead)
          {
            mzx_world->bomb_type ^= 1;
            if((!src_board->player_attack_locked) &&
             (src_board->can_bomb))
            {
              play_sfx(mzx_world, 35);
              if(mzx_world->bomb_type)
              {
                set_mesg(mzx_world,
                 "You switch to high strength bombs.");
              }
              else
              {
                set_mesg(mzx_world,
                 "You switch to low strength bombs.");
              }
            }
          }
          break;
        }

        case SDLK_F6:
        {
          if(mzx_world->editing)
          {
            // Debug information
            debug_mode ^= 1;
            break;
          }
        }

        case SDLK_F7:
        {
          if(mzx_world->editing)
          {
            int i;

            // Cheat #1- Give all
            set_counter(mzx_world, "GEMS", 32767, 1);
            set_counter(mzx_world, "AMMO", 32767, 1);
            set_counter(mzx_world, "HEALTH", 32767, 1);
            set_counter(mzx_world, "COINS", 32767, 1);
            set_counter(mzx_world, "LIVES", 32767, 1);
            set_counter(mzx_world, "TIME", src_board->time_limit, 1);
            set_counter(mzx_world, "LOBOMBS", 32767, 1);
            set_counter(mzx_world, "HIBOMBS", 32767, 1);

            mzx_world->score = 0;
            mzx_world->dead = 0;

            for(i = 0; i < 16; i++)
            {
              mzx_world->keys[i] = i;
            }

            src_board->player_ns_locked = 0;
            src_board->player_ew_locked = 0;
            src_board->player_attack_locked = 0;
          }

          break;
        }

        case SDLK_F8:
        {
          if(mzx_world->editing)
          {
            int player_x = mzx_world->player_x;
            int player_y = mzx_world->player_y;
            int board_width = src_board->board_width;
            int board_height = src_board->board_height;

            if(player_x > 0)
            {
              place_at_xy(mzx_world, SPACE, 7, 0, player_x - 1,
               player_y);

              if(player_y > 0)
              {
                place_at_xy(mzx_world, SPACE, 7, 0, player_x - 1,
                 player_y - 1);
              }

              if(player_y < (board_height - 1))
              {
                place_at_xy(mzx_world, SPACE, 7, 0, player_x - 1,
                 player_y + 1);
              }
            }

            if(player_x < (board_width - 1))
            {
              place_at_xy(mzx_world, SPACE, 7, 0, player_x + 1,
               player_y);

              if(player_y > 0)
              {
                place_at_xy(mzx_world, SPACE, 7, 0,
                 player_x + 1, player_y - 1);
              }

              if(player_y < (board_height - 1))
              {
                place_at_xy(mzx_world, SPACE, 7, 0,
                 player_x + 1, player_y + 1);
              }
            }

            if(player_y > 0)
            {
              place_at_xy(mzx_world, SPACE, 7, 0,
               player_x, player_y - 1);
            }

            if(player_y < (board_height - 1))
            {
              place_at_xy(mzx_world, SPACE, 7, 0,
               player_x, player_y + 1);
            }
          }

          break;
        }

        // Quick save
        case SDLK_F9:
        {
          if(!mzx_world->dead)
          {
            // Can we?
            if((src_board->save_mode != CANT_SAVE) &&
             ((src_board->save_mode != CAN_SAVE_ON_SENSOR) ||
             (src_board->level_under_id[mzx_world->player_x +
             (src_board->board_width * mzx_world->player_y)] ==
             SENSOR)))
            {
              // Save entire game
              save_world(mzx_world, curr_sav, 1, get_fade_status());
            }
          }
          break;
        }

        // Quick load
        case SDLK_F10:
        {
          struct stat file_info;

          if(!stat(curr_sav, &file_info))
          {
            // Load game
            fadein = 0;
            if(reload_savegame(mzx_world, curr_sav, &fadein))
            {
              vquick_fadeout();
              return;
            }

            // Reset this
            src_board = mzx_world->current_board;

            find_player(mzx_world);

            // Swap in starting board
            load_mod(src_board->mod_playing);
            strcpy(mzx_world->real_mod_playing,
             src_board->mod_playing);

            send_robot_def(mzx_world, 0, 10);
            fadein ^= 1;
          }
          break;
        }

        case SDLK_F11:
        {
          if(mzx_world->editing)
            debug_counters(mzx_world);

          break;
        }
      }
    }
  } while(key != SDLK_ESCAPE);

  pop_context();
  vquick_fadeout();
  clear_sfx_queue();
}

void place_player(World *mzx_world, int x, int y, int dir)
{
  Board *src_board = mzx_world->current_board;
  if((mzx_world->player_x != x) || (mzx_world->player_y != y))
  {
    id_remove_top(mzx_world, mzx_world->player_x, mzx_world->player_y);
  }
  id_place(mzx_world, x, y, PLAYER, 0, 0);
  mzx_world->player_x = x;
  mzx_world->player_y = y;
  src_board->player_last_dir =
   (src_board->player_last_dir & 240) | (dir + 1);
}

// Returns 1 if didn't move
int move_player(World *mzx_world, int dir)
{
  Board *src_board = mzx_world->current_board;
  // Dir is from 0 to 3
  int player_x = mzx_world->player_x;
  int player_y = mzx_world->player_y;
  int new_x = player_x;
  int new_y = player_y;
  int edge = 0;

  switch(dir)
  {
    case 0:
      if(--new_y < 0)
        edge = 1;
      break;
    case 1:
      if(++new_y >= src_board->board_height)
        edge = 1;
      break;
    case 2:
      if(++new_x >= src_board->board_width)
        edge = 1;
      break;
    case 3:
      if(--new_x < 0)
        edge = 1;
      break;
  }

  if(edge)
  {
    // Hit an edge, teleport to another board?
    int board_dir = src_board->board_dir[dir];
    // Board must be valid
    if((board_dir == NO_BOARD) ||
     (board_dir >= mzx_world->num_boards) ||
     (!mzx_world->board_list[board_dir]))
    {
      return 1;
    }

    mzx_world->target_board = board_dir;
    mzx_world->target_where = TARGET_POSITION;
    mzx_world->target_x = player_x;
    mzx_world->target_y = player_y;

    switch(dir)
    {
      case 0: // North- Enter south side
      {
        mzx_world->target_y =
         (mzx_world->board_list[board_dir])->board_height - 1;
        break;
      }

      case 1: // South- Enter north side
      {
        mzx_world->target_y = 0;
        break;
      }

      case 2: // East- Enter west side
      {
        mzx_world->target_x = 0;
        break;
      }

      case 3: // West- Enter east side
      {
        mzx_world->target_x =
         (mzx_world->board_list[board_dir])->board_width - 1;
        break;
      }
    }
    src_board->player_last_dir =
     (src_board->player_last_dir & 240) + dir + 1;
    return 0;
  }
  else
  {
    // Not edge
    int d_offset = new_x + (new_y * src_board->board_width);
    mzx_thing d_id = (mzx_thing)src_board->level_id[d_offset];
    int d_flag = flags[(int)d_id];

    if(d_flag & A_SPEC_STOOD)
    {
      // Sensor
      // Activate label and then move player
      int d_param = src_board->level_param[d_offset];
      send_robot(mzx_world,
       (src_board->sensor_list[d_param])->robot_to_mesg,
       "SENSORON", 0);
      place_player(mzx_world, new_x, new_y, dir);
    }
    else

    if(d_flag & A_ENTRANCE)
    {
      // Entrance
      int d_board = src_board->level_param[d_offset];
      play_sfx(mzx_world, 37);
      // Can move?
      if((d_board != mzx_world->current_board_id) &&
       (d_board < mzx_world->num_boards) &&
       (mzx_world->board_list[d_board]))
      {
        // Go to board t1 AFTER showing update
        mzx_world->target_board = d_board;
        mzx_world->target_where = TARGET_ENTRANCE;
        mzx_world->target_color = src_board->level_color[d_offset];
        mzx_world->target_id = d_id;
      }

      place_player(mzx_world, new_x, new_y, dir);
    }
    else

    if((d_flag & A_ITEM) && (d_id != ROBOT_PUSHABLE))
    {
      // Item
      mzx_thing d_under_id = (mzx_thing)mzx_world->under_player_id;
      char d_under_color = mzx_world->under_player_color;
      char d_under_param = mzx_world->under_player_param;
      int grab_result = grab_item(mzx_world, d_offset, dir);
      if(grab_result)
      {
        if(d_id == TRANSPORT)
        {
          int player_last_dir = src_board->player_last_dir;
          // Teleporter
          id_remove_top(mzx_world, player_x, player_y);
          mzx_world->under_player_id = (char)d_under_id;
          mzx_world->under_player_color = d_under_color;
          mzx_world->under_player_param = d_under_param;
          src_board->player_last_dir =
           (player_last_dir & 240) + dir + 1;
          // New player x/y will be found after update !!! maybe fix.
        }
        else
        {
          place_player(mzx_world, new_x, new_y, dir);
        }
        return 0;
      }
      return 1;
    }
    else

    if(d_flag & A_UNDER)
    {
      // Under
      place_player(mzx_world, new_x, new_y, dir);
      return 0;
    }
    else

    if((d_flag & A_ENEMY) || (d_flag & A_HURTS))
    {
      if(d_id == BULLET)
      {
        // Bullet
        if((src_board->level_param[d_offset] >> 2) == PLAYER_BULLET)
        {
          // Player bullet no hurty
          id_remove_top(mzx_world, new_x, new_y);
          place_player(mzx_world, new_x, new_y, dir);
          return 0;
        }
        else
        {
          // Enemy or hurtsy
          dec_counter(mzx_world, "HEALTH", id_dmg[61], 0);
          play_sfx(mzx_world, 21);
          set_mesg(mzx_world, "Ouch!");
        }
      }
      else
      {
        dec_counter(mzx_world, "HEALTH", id_dmg[(int)d_id], 0);
        play_sfx(mzx_world, 21);
        set_mesg(mzx_world, "Ouch!");

        if(d_flag & A_ENEMY)
        {
          // Kill/move
          id_remove_top(mzx_world, new_x, new_y);
          if((d_id != GOOP) &&
           (!src_board->restart_if_zapped)) // Not onto goop
          if(!src_board->restart_if_zapped)
          {
            place_player(mzx_world, new_x, new_y, dir);
            return 0;
          }
        }
      }
    }
    else
    {
      int dir_mask;
      // Check for push
      if(dir > 1)
        dir_mask = d_flag & A_PUSHEW;
      else
        dir_mask = d_flag & A_PUSHNS;

      if(dir_mask || (d_flag & A_SPEC_PUSH))
      {
        // Push
        // Pushable robot needs to be sent the touch label
        if(d_id == ROBOT_PUSHABLE)
          send_robot_def(mzx_world,
           src_board->level_param[d_offset], 0);

        if(!push(mzx_world, player_x, player_y, dir, 0))
        {
          place_player(mzx_world, new_x, new_y, dir);
          return 0;
        }
      }
    }
    // Nothing.
  }
  return 1;
}

void give_potion(World *mzx_world, mzx_potion type)
{
  Board *src_board = mzx_world->current_board;
  int board_width = src_board->board_width;
  int board_height = src_board->board_height;
  char *level_id = src_board->level_id;
  char *level_param = src_board->level_param;
  char *level_color = src_board->level_color;

  play_sfx(mzx_world, 39);
  inc_counter(mzx_world, "SCORE", 5, 0);

  switch(type)
  {
    case POTION_DUD:
    {
      set_mesg(mzx_world, "* No effect *");
      break;
    }

    case POTION_INVINCO:
    {
      set_mesg(mzx_world, "* Invinco *");
      send_robot_def(mzx_world, 0, 2);
      set_counter(mzx_world, "INVINCO", 113, 0);
      break;
    }

    case POTION_BLAST:
    {
      int x, y, offset;
      mzx_thing d_id;
      int d_flag;

      // Set the placement rate
      int placement_rate = 18;
      for(y = 0, offset = 0; y < board_height; y++)
      {
        for(x = 0; x < board_width; x++, offset++)
        {
          d_id = (mzx_thing)level_id[offset];
          d_flag = flags[d_id];

          if((d_flag & A_UNDER) && !(d_flag & A_ENTRANCE))
          {
            // Adjust the ratio for board size

            if((rand() % placement_rate) == 0)
            {
              id_place(mzx_world, x, y, EXPLOSION, 0,
               16 * ((rand() % 5) + 2));
            }
          }
        }
      }

      set_mesg(mzx_world, "* Blast *");
      break;
    }

    case POTION_SMALL_HEALTH:
    {
      inc_counter(mzx_world, "HEALTH", 10, 0);
      set_mesg(mzx_world, "* Healing *");
      break;
    }

    case POTION_BIG_HEALTH:
    {
      inc_counter(mzx_world, "HEALTH", 50, 0);
      set_mesg(mzx_world, "* Healing *");
      break;
    }

    case POTION_POISON:
    {
      dec_counter(mzx_world, "HEALTH", 10, 0);
      set_mesg(mzx_world, "* Poison *");
      break;
    }

    case POTION_BLIND:
    {
      mzx_world->blind_dur = 200;
      set_mesg(mzx_world, "* Blind *");
      break;
    }

    case POTION_KILL:
    {
      int x, y, offset;
      mzx_thing d_id;

      for(y = 0, offset = 0; y < board_height; y++)
      {
        for(x = 0; x < board_width; x++, offset++)
        {
          d_id = (mzx_thing)level_id[offset];

          if(is_enemy(d_id))
            id_remove_top(mzx_world, x, y);
        }
      }
      set_mesg(mzx_world, "* Kill enemies *");
      break;
    }

    case POTION_FIREWALK:
    {
      mzx_world->firewalker_dur = 200;
      set_mesg(mzx_world, "* Firewalking *");
      break;
    }

    case POTION_DETONATE:
    {
      int x, y, offset;
      for(y = 0, offset = 0; y < board_height; y++)
      {
        for(x = 0; x < board_width; x++, offset++)
        {
          if(level_id[offset] == BOMB)
          {
            level_id[offset] = EXPLOSION;
            if(level_param[offset] == 0)
              level_param[offset] = 32;
            else
              level_param[offset] = 64;
          }
        }
      }
      set_mesg(mzx_world, "* Detonate *");
      break;
    }

    case POTION_BANISH:
    {
      int x, y, offset;
      for(y = 0, offset = 0; y < board_height; y++)
      {
        for(x = 0; x < board_width; x++, offset++)
        {
          if(level_id[offset] == (char)DRAGON)
          {
            level_id[offset] = GHOST;
            level_param[offset] = 51;
          }
        }
      }
      set_mesg(mzx_world, "* Banish dragons *");
      break;
    }

    case POTION_SUMMON:
    {
      mzx_thing d_id;
      int x, y, offset;

      for(y = 0, offset = 0; y < board_height; y++)
      {
        for(x = 0; x < board_width; x++, offset++)
        {
          d_id = (mzx_thing)level_id[offset];
          if(is_enemy(d_id))
          {
            level_id[offset] = (char)DRAGON;
            level_color[offset] = 4;
            level_param[offset] = 102;
          }
        }
      }
      set_mesg(mzx_world, "* Summon dragons *");
      break;
    }

    case POTION_AVALANCHE:
    {
      int x, y, offset;
      // Adjust the rate for board size - it was hardcoded for 10000
      int placement_rate = 18 * (board_width * board_height) / 10000;

      for(y = 0, offset = 0; y < board_height; y++)
      {
        for(x = 0; x < board_width; x++, offset++)
        {
          int d_flag = flags[(int)level_id[offset]];

          if((d_flag & A_UNDER) && !(d_flag & A_ENTRANCE))
          {
            if((rand() % placement_rate) == 0)
            {
              id_place(mzx_world, x, y, BOULDER, 7, 0);
            }
          }
        }
      }
      set_mesg(mzx_world, "* Avalanche *");
      break;
    }

    case POTION_FREEZE:
    {
      mzx_world->freeze_time_dur = 200;
      set_mesg(mzx_world, "* Freeze time *");
      break;
    }

    case POTION_WIND:
    {
      mzx_world->wind_dur = 200;
      set_mesg(mzx_world, "* Wind *");
      break;
    }

    case POTION_SLOW:
    {
      mzx_world->slow_time_dur = 200;
      set_mesg(mzx_world, "* Slow time *");
      break;
    }
  }
}

int grab_item(World *mzx_world, int offset, int dir)
{
  // Dir is for transporter
  Board *src_board = mzx_world->current_board;
  mzx_thing id = (mzx_thing)src_board->level_id[offset];
  char param = src_board->level_param[offset];
  char color = src_board->level_color[offset];
  int remove = 0;

  char tmp[81];

  switch(id)
  {
    case CHEST: // Chest
    {
      int item;

      if(!(param & 15))
      {
        play_sfx(mzx_world, 40);
        break;
      }

      // Act upon contents
      play_sfx(mzx_world, 41);
      item = ((param & 240) >> 4); // Amount for most things

      switch((mzx_chest_contents)(param & 15))
      {
        case ITEM_KEY: // Key
        {
          if(give_key(mzx_world, item))
          {
            set_mesg(mzx_world, "Inside the chest is a key, "
             "but you can't carry any more keys!");
            return 0;
          }
          set_mesg(mzx_world, "Inside the chest you find a key.");
          break;
        }

        case ITEM_COINS: // Coins
        {
          item *= 5;
          set_3_mesg(mzx_world, "Inside the chest you find ",
           item, " coins.");
          inc_counter(mzx_world, "COINS", item, 0);
          inc_counter(mzx_world, "SCORE", item, 0);
          break;
        }

        case ITEM_LIFE: // Life
        {
          if(item > 1)
          {
            set_3_mesg(mzx_world, "Inside the chest you find ",
             item, " free lives.");
          }
          else
          {
            set_mesg(mzx_world,
             "Inside the chest you find 1 free life.");
          }
          inc_counter(mzx_world, "LIVES", item, 0);
          break;
        }

        case ITEM_AMMO: // Ammo
        {
          item *= 5;
          set_3_mesg(mzx_world,
           "Inside the chest you find ", item, " rounds of ammo.");
          inc_counter(mzx_world, "AMMO", item, 0);
          break;
        }

        case ITEM_GEMS: // Gems
        {
          item *= 5;
          set_3_mesg(mzx_world, "Inside the chest you find ",
           item, " gems.");
          inc_counter(mzx_world, "GEMS", item, 0);
          inc_counter(mzx_world, "SCORE", item, 0);
          break;
        }

        case ITEM_HEALTH: // Health
        {
          item *= 5;
          set_3_mesg(mzx_world, "Inside the chest you find ",
           item, " health.");
          inc_counter(mzx_world, "HEALTH", item, 0);
          break;
        }

        case ITEM_POTION: // Potion
        {
          int answer;
          m_show();
          answer = confirm(mzx_world,
           "Inside the chest you find a potion. Drink it?");

          if(answer)
            return 0;

          src_board->level_param[offset] = 0;
          give_potion(mzx_world, (mzx_potion)item);
          break;
        }

        case ITEM_RING: // Ring
        {
          int answer;

          m_show();

          answer = confirm(mzx_world,
           "Inside the chest you find a ring. Wear it?");

          if(answer)
            return 0;

          src_board->level_param[offset] = 0;
          give_potion(mzx_world, (mzx_potion)item);
          break;
        }

        case ITEM_LOBOMBS: // Lobombs
        {
          item *= 5;
          set_3_mesg(mzx_world, "Inside the chest you find ", item,
           " low strength bombs.");
          inc_counter(mzx_world, "LOBOMBS", item, 0);
          break;
        }

        case ITEM_HIBOMBS: // Hibombs
        {
          item *= 5;
          set_3_mesg(mzx_world, "Inside the chest you find ", item,
           " high strength bombs.");
          inc_counter(mzx_world, "HIBOMBS", item, 0);
          break;
        }
      }
      // Empty chest
      src_board->level_param[offset] = 0;
      break;
    }

    case GEM:
    case MAGIC_GEM:
    {
      play_sfx(mzx_world, id - 28);
      inc_counter(mzx_world, "GEMS", 1, 0);

      if(id == MAGIC_GEM)
        inc_counter(mzx_world, "HEALTH", 1, 0);

      inc_counter(mzx_world, "SCORE", 1, 0);
      remove = 1;
      break;
    }

    case HEALTH:
    {
      play_sfx(mzx_world, 2);
      inc_counter(mzx_world, "HEALTH", param, 0);
      remove = 1;
      break;
    }

    case RING:
    case POTION:
    {
      give_potion(mzx_world, (mzx_potion)param);
      remove = 1;
      break;
    }

    case GOOP:
    {
      play_sfx(mzx_world, 48);
      set_mesg(mzx_world, "There is goop in your way!");
      send_robot_def(mzx_world, 0, 12);
      break;
    }

    case ENERGIZER:
    {
      play_sfx(mzx_world, 16);
      set_mesg(mzx_world, "Energize!");
      send_robot_def(mzx_world, 0, 2);
      set_counter(mzx_world, "INVINCO", 113, 0);
      remove = 1;
      break;
    }

    case AMMO:
    {
      play_sfx(mzx_world, 3);
      inc_counter(mzx_world, "AMMO", param, 0);
      remove = 1;
      break;
    }

    case BOMB:
    {
      if(src_board->collect_bombs)
      {
        if(param)
        {
          play_sfx(mzx_world, 7);
          inc_counter(mzx_world, "HIBOMBS", 1, 0);
        }
        else
        {
          play_sfx(mzx_world, 6);
          inc_counter(mzx_world, "LOBOMBS", 1, 0);
        }
        remove = 1;
      }
      else
      {
        // Light bomb
        play_sfx(mzx_world, 33);
        src_board->level_id[offset] = 37;
        src_board->level_param[offset] = param << 7;
      }
      break;
    }

    case KEY:
    {
      if(give_key(mzx_world, color))
      {
        play_sfx(mzx_world, 9);
        set_mesg(mzx_world, "You can't carry any more keys!");
      }
      else
      {
        play_sfx(mzx_world, 8);
        set_mesg(mzx_world, "You grab the key.");
        remove = 1;
      }
      break;
    }

    case LOCK:
    {
      if(take_key(mzx_world, color))
      {
        play_sfx(mzx_world, 11);
        set_mesg(mzx_world, "You need an appropriate key!");
      }
      else
      {
        play_sfx(mzx_world, 10);
        set_mesg(mzx_world, "You open the lock.");
        remove = 1;
      }
      break;
    }

    case DOOR:
    {
      int board_width = src_board->board_width;
      char *level_id = src_board->level_id;
      char *level_param = src_board->level_param;
      int x = offset % board_width;
      int y = offset / board_width;
      char door_first_movement[8] = { 0, 3, 0, 2, 1, 3, 1, 2 };

      if(param & 8)
      {
        // Locked
        if(take_key(mzx_world, color))
        {
          // Need key
          play_sfx(mzx_world, 19);
          set_mesg(mzx_world, "The door is locked!");
          break;
        }

        // Unlocked
        set_mesg(mzx_world, "You unlock and open the door.");
      }
      else
      {
        set_mesg(mzx_world, "You open the door.");
      }

      src_board->level_id[offset] = 42;
      src_board->level_param[offset] = (param & 7);

      if(move(mzx_world, x, y, door_first_movement[param & 7],
       CAN_PUSH | CAN_LAVAWALK | CAN_FIREWALK | CAN_WATERWALK))
      {
        set_mesg(mzx_world, "The door is blocked from opening!");
        play_sfx(mzx_world, 19);
        level_id[offset] = 41;
        level_param[offset] = param & 7;
      }
      else
      {
        play_sfx(mzx_world, 20);
      }
      break;
    }

    case GATE:
    {
      if(param)
      {
        // Locked
        if(take_key(mzx_world, color))
        {
          // Need key
          play_sfx(mzx_world, 14);
          set_mesg(mzx_world, "The gate is locked!");
          break;
        }
        // Unlocked
        set_mesg(mzx_world, "You unlock and open the gate.");
      }
      else
      {
        set_mesg(mzx_world, "You open the gate.");
      }

      src_board->level_id[offset] = (char)OPEN_GATE;
      src_board->level_param[offset] = 22;
      play_sfx(mzx_world, 15);
      break;
    }

    case TRANSPORT:
    {
      int x = offset % src_board->board_width;
      int y = offset / src_board->board_width;

      if(transport(mzx_world, x, y, dir, PLAYER, 0, 0, 1))
        break;

      return 1;
    }

    case COIN:
    {
      play_sfx(mzx_world, 4);
      inc_counter(mzx_world, "COINS", 1, 0);
      inc_counter(mzx_world, "SCORE", 1, 0);
      remove = 1;
      break;
    }

    case POUCH:
    {
      play_sfx(mzx_world, 38);
      inc_counter(mzx_world, "GEMS", (param & 15) * 5, 0);
      inc_counter(mzx_world, "COINS", (param >> 4) * 5, 0);
      inc_counter(mzx_world, "SCORE", ((param & 15) + (param >> 4)) * 5, 1);
      sprintf(tmp, "The pouch contains %d gems and %d coins.",
       (param & 15) * 5, (param >> 4) * 5);
      set_mesg(mzx_world, tmp);
      remove = 1;
      break;
    }

    case FOREST:
    {
      play_sfx(mzx_world, 13);
      if(src_board->forest_becomes == FOREST_TO_EMPTY)
      {
        remove = 1;
      }
      else
      {
        src_board->level_id[offset] = (char)FLOOR;
        return 1;
      }
      break;
    }

    case LIFE:
    {
      play_sfx(mzx_world, 5);
      inc_counter(mzx_world, "LIVES", 1, 0);
      set_mesg(mzx_world, "You find a free life!");
      remove = 1;
      break;
    }

    case INVIS_WALL:
    {
      src_board->level_id[offset] = (char)NORMAL;
      set_mesg(mzx_world, "Oof! You ran into an invisible wall.");
      play_sfx(mzx_world, 12);
      break;
    }

    case MINE:
    {
      src_board->level_id[offset] = (char)EXPLOSION;
      src_board->level_param[offset] = param & 240;
      play_sfx(mzx_world, 36);
      break;
    }

    case EYE:
    {
      src_board->level_id[offset] = (char)EXPLOSION;
      src_board->level_param[offset] = (param << 1) & 112;
      break;
    }

    case THIEF:
    {
      dec_counter(mzx_world, "GEMS", (param & 128) >> 7, 0);
      play_sfx(mzx_world, 44);
      break;
    }

    case SLIMEBLOB:
    {
      if(param & 64)
        hurt_player_id(mzx_world, SLIMEBLOB);

      if(param & 128)
        break;

      src_board->level_id[offset] = (char)BREAKAWAY;
      break;
    }

    case GHOST:
    {
      hurt_player_id(mzx_world, GHOST);

      // Die !?
      if(!(param & 8))
        remove = 1;

      break;
    }

    case DRAGON:
    {
      hurt_player_id(mzx_world, DRAGON);
      break;
    }

    case FISH:
    {
      if(param & 64)
      {
        hurt_player_id(mzx_world, FISH);
        remove = 1;
      }
      break;
    }

    case ROBOT:
    {
      send_robot_def(mzx_world, param, 0);
      break;
    }

    case SIGN:
    case SCROLL:
    {
      int idx = param;
      play_sfx(mzx_world, 47);

      m_show();
      scroll_edit(mzx_world, src_board->scroll_list[idx], id & 1);

      if(id == SCROLL)
        remove = 1;
      break;
    }

    default:
      break;
  }

  if(remove == 1)
    offs_remove_id(mzx_world, offset);

  return remove; // Not grabbed
}

// Show status screen
void show_status(World *mzx_world)
{
  int i;
  char temp[11];
  char (*status_counters_shown)[15] = mzx_world->status_counters_shown;
  char *keys = mzx_world->keys;

  draw_window_box(37, 4, 67, 21, 25, 16, 24, 1, 1);
  show_counter(mzx_world, "Gems", 39, 5, 0);
  show_counter(mzx_world, "Ammo", 39, 6, 0);
  show_counter(mzx_world, "Health", 39, 7, 0);
  show_counter(mzx_world, "Lives", 39, 8, 0);
  show_counter(mzx_world, "Lobombs", 39, 9, 0);
  show_counter(mzx_world, "Hibombs", 39, 10, 0);
  show_counter(mzx_world, "Coins", 39, 11, 0);

  for(i = 0; i < NUM_STATUS_CNTRS; i++) // Show custom status counters
  {
    if(status_counters_shown[i][0])
      show_counter(mzx_world, status_counters_shown[i], 39, i + 15, 1);
  }

  write_string("Score", 39, 12, 27, 0);
  sprintf(temp, "%d", mzx_world->score);
  write_string(temp, 55, 12, 31, 0); // Show score
  write_string("Keys", 39, 13, 27, 0);

  for(i = 0; i < 8; i++) // Show keys
  {
    if(keys[i] != NO_KEY)
      draw_char('', 16 + keys[i], 55 + i, 13);
  }

  for(; i < 16; i++) // Show keys, 2nd row
  {
    if(keys[i] != NO_KEY)
      draw_char('', 16 + keys[i], 47 + i, 14);
  }

  // Show hi/lo bomb selection

  write_string("(cur.)", 48, 9 + mzx_world->bomb_type, 25, 0);
  // Show effects
  if(mzx_world->firewalker_dur > 0)
    write_string("-W-", 43, 21, 28, 0);

  if(mzx_world->freeze_time_dur > 0)
    write_string("-F-", 47, 21, 27, 0);

  if(mzx_world->slow_time_dur > 0)
    write_string("-S-", 51, 21, 30, 0);

  if(mzx_world->wind_dur > 0)
    write_string("-W-", 55, 21, 31, 0);

  if(mzx_world->blind_dur > 0)
    write_string("-B-", 59, 21, 25, 0);
}

void show_counter(World *mzx_world, char *str, int x, int y,
 int skip_if_zero)
{
  int counter_value = get_counter(mzx_world, str, 0);
  if((skip_if_zero) && (!counter_value))
    return;
  write_string(str, x, y, 27, 0); // Name

  write_number(counter_value, 31, x + 16, y, 1, 0, 10); // Value
}

// Returns non-0 to skip all keys this cycle
int update(World *mzx_world, int game, int *fadein)
{
  int start_ticks = get_ticks();
  int time_remaining;
  static int reload = 0;
  static int slowed = 0; // Flips between 0 and 1 during slow_time
  char tmp_str[10];
  Board *src_board = mzx_world->current_board;
  int volume = src_board->volume;
  int volume_inc = src_board->volume_inc;
  int volume_target = src_board->volume_target;
  int board_width = src_board->board_width;
  int board_height = src_board->board_height;
  char *level_id = src_board->level_id;
  char *level_color = src_board->level_color;
  char *level_param = src_board->level_param;
  char *level_under_id = src_board->level_under_id;
  char *level_under_color = src_board->level_under_color;
  char *level_under_param = src_board->level_under_param;
  int total_ticks;

  pal_update = 0;

  if((game) && get_counter(mzx_world, "CURSORSTATE", 0))
  {
    // Turn on mouse
    m_show();
  }
  else
  {
    // Turn off mouse
    m_hide();
  }

  // Fade mod
  if(volume_inc)
  {
    int result_volume = volume;
    result_volume += volume_inc;
    if(volume_inc < 0)
    {
      if(result_volume <= volume_target)
      {
        result_volume = volume_target;
        volume_inc = 0;
      }
    }
    else
    {
      if(result_volume >= volume_target)
      {
        result_volume = volume_target;
        volume_inc = 0;
      }
    }
    src_board->volume = result_volume;
    volume_mod(volume);
  }

  // Slow_time- flip slowed
  if(mzx_world->freeze_time_dur)
    slowed = 1;
  else

  if(mzx_world->slow_time_dur)
    slowed ^= 1;
  else
    slowed = 0;

  // Update
  update_variables(mzx_world, slowed);
  update_player(mzx_world); // Ice, fire, water, lava

  if(mzx_world->wind_dur > 0)
  {
    // Wind
    int wind_dir = rand() % 9;
    if(wind_dir < 4)
    {
      // No wind this turn if above 3
      src_board->player_last_dir =
       (src_board->player_last_dir & 0xF0) + wind_dir;
      move_player(mzx_world, wind_dir);
    }
  }

  // The following is during gameplay ONLY
  if(game && (!mzx_world->dead))
  {
    // Shoot
    if(get_key_status(keycode_SDL, SDLK_SPACE))
    {
      if((!reload) && (!src_board->player_attack_locked))
      {
        int move_dir = -1;

        if(get_key_status(keycode_SDL, SDLK_UP))
          move_dir = 0;
        else

        if(get_key_status(keycode_SDL, SDLK_DOWN))
          move_dir = 1;
        else

        if(get_key_status(keycode_SDL, SDLK_RIGHT))
          move_dir = 2;
        else

        if(get_key_status(keycode_SDL, SDLK_LEFT))
          move_dir = 3;

        if(move_dir != -1)
        {
          if(!src_board->can_shoot)
            set_mesg(mzx_world, "Can't shoot here!");
          else

          if(!get_counter(mzx_world, "AMMO", 0))
          {
            set_mesg(mzx_world, "You are out of ammo!");
            play_sfx(mzx_world, 30);
          }
          else
          {
            dec_counter(mzx_world, "AMMO", 1, 0);
            play_sfx(mzx_world, 28);
            shoot(mzx_world, mzx_world->player_x, mzx_world->player_y,
             move_dir, PLAYER_BULLET);
            reload = 2;
            src_board->player_last_dir =
             (src_board->player_last_dir & 0x0F) | (move_dir << 4);
          }
        }
      }
    }
    else

    if((get_key_status(keycode_SDL, SDLK_UP)) &&
     (!src_board->player_ns_locked))
    {
      int key_up_delay = mzx_world->key_up_delay;
      if((key_up_delay == 0) || (key_up_delay > REPEAT_WAIT))
      {
        move_player(mzx_world, 0);
        src_board->player_last_dir = (src_board->player_last_dir & 0x0F);
      }
      if(key_up_delay <= REPEAT_WAIT)
        mzx_world->key_up_delay = key_up_delay + 1;
    }
    else

    if((get_key_status(keycode_SDL, SDLK_DOWN)) &&
     (!src_board->player_ns_locked))
    {
      int key_down_delay = mzx_world->key_down_delay;
      if((key_down_delay == 0) || (key_down_delay > REPEAT_WAIT))
      {
        move_player(mzx_world, 1);
        src_board->player_last_dir =
         (src_board->player_last_dir & 0x0F) + 0x10;
      }
      if(key_down_delay <= REPEAT_WAIT)
        mzx_world->key_down_delay = key_down_delay + 1;
    }
    else

    if((get_key_status(keycode_SDL, SDLK_RIGHT)) &&
     (!src_board->player_ew_locked))
    {
      int key_right_delay = mzx_world->key_right_delay;
      if((key_right_delay == 0) || (key_right_delay > REPEAT_WAIT))
      {
        move_player(mzx_world, 2);
        src_board->player_last_dir =
         (src_board->player_last_dir & 0x0F) + 0x20;
      }
      if(key_right_delay <= REPEAT_WAIT)
        mzx_world->key_right_delay = key_right_delay + 1;
    }
    else

    if((get_key_status(keycode_SDL, SDLK_LEFT)) &&
     (!src_board->player_ew_locked))
    {
      int key_left_delay = mzx_world->key_left_delay;
      if((key_left_delay == 0) || (key_left_delay > REPEAT_WAIT))
      {
        move_player(mzx_world, 3);
        src_board->player_last_dir =
         (src_board->player_last_dir & 0x0F) + 0x30;
      }
      if(key_left_delay <= REPEAT_WAIT)
        mzx_world->key_left_delay = key_left_delay + 1;
    }
    else
    {
      mzx_world->key_up_delay = 0;
      mzx_world->key_down_delay = 0;
      mzx_world->key_right_delay = 0;
      mzx_world->key_left_delay = 0;
    }

    // Bomb
    if(get_key_status(keycode_SDL, SDLK_DELETE) &&
     (!src_board->player_attack_locked))
    {
      int d_offset =
       mzx_world->player_x + (mzx_world->player_y * board_width);

      if(level_under_id[d_offset] != (char)LIT_BOMB)
      {
        // Okay.
        if(!src_board->can_bomb)
        {
          set_mesg(mzx_world, "Can't bomb here!");
        }
        else

        if((mzx_world->bomb_type == 0) &&
         (!get_counter(mzx_world, "LOBOMBS", 0)))
        {
          set_mesg(mzx_world, "You are out of low strength bombs!");
          play_sfx(mzx_world, 32);
        }
        else

        if((mzx_world->bomb_type == 1) &&
         (!get_counter(mzx_world, "HIBOMBS", 0)))
        {
          set_mesg(mzx_world, "You are out of high strength bombs!");
          play_sfx(mzx_world, 32);
        }
        else

        if(!(flags[(int)level_under_id[d_offset]] & A_ENTRANCE))
        {
          // Bomb!
          mzx_world->under_player_id = level_under_id[d_offset];
          mzx_world->under_player_color = level_under_color[d_offset];
          mzx_world->under_player_param = level_under_param[d_offset];
          level_under_id[d_offset] = 37;
          level_under_color[d_offset] = 8;
          level_under_param[d_offset] = mzx_world->bomb_type << 7;
          play_sfx(mzx_world, 33 + mzx_world->bomb_type);

          if(mzx_world->bomb_type)
            dec_counter(mzx_world, "HIBOMBS", 1, 0);
          else
            dec_counter(mzx_world, "LOBOMBS", 1, 0);
        }
      }
    }
    if(reload) reload--;
  }

  mzx_world->swapped = 0;

  if((src_board->robot_list[0] != NULL) &&
   (src_board->robot_list[0])->used)
  {
    run_robot(mzx_world, 0, -1, -1);
    src_board = mzx_world->current_board;
    level_under_id = src_board->level_under_id;
    board_width = src_board->board_width;
  }

  if(!slowed)
  {
    int entrance = 1;
    int d_offset =
     mzx_world->player_x + (mzx_world->player_y * board_width);

    mzx_world->was_zapped = 0;
    if(flags[(int)level_under_id[d_offset]] & A_ENTRANCE)
      entrance = 0;

    update_board(mzx_world);

    src_board = mzx_world->current_board;
    level_under_id = src_board->level_under_id;
    board_width = src_board->board_width;
    d_offset = mzx_world->player_x + (mzx_world->player_y * board_width);

    // Pushed onto an entrance?

    if((entrance) && (flags[(int)level_under_id[d_offset]] & A_ENTRANCE)
     && (!mzx_world->was_zapped))
    {
      int d_board = src_board->level_under_param[d_offset];
      clear_sfx_queue(); // Since there is often a push sound
      play_sfx(mzx_world, 37);

      // Same board or nonexistant?
      if((d_board != mzx_world->current_board_id)
       && (d_board < mzx_world->num_boards) &&
       (mzx_world->board_list[d_board] != NULL))
      {
        // Nope.
        // Go to board d_board
        mzx_world->target_board = d_board;
        mzx_world->target_where = TARGET_ENTRANCE;
        mzx_world->target_color = level_under_color[d_offset];
        mzx_world->target_id = (mzx_thing)level_under_id[d_offset];
      }
    }

    mzx_world->was_zapped = 0;
  }

  // Death and game over
  if(get_counter(mzx_world, "LIVES", 0) == 0)
  {
    int endgame_board = mzx_world->endgame_board;
    int endgame_x = mzx_world->endgame_x;
    int endgame_y = mzx_world->endgame_y;

    // Game over
    if(endgame_board != NO_ENDGAME_BOARD)
    {
      // Jump to given board
      if(mzx_world->current_board_id == endgame_board)
      {
        id_remove_top(mzx_world, mzx_world->player_x,
         mzx_world->player_y);
        id_place(mzx_world, endgame_x, endgame_y, PLAYER, 0, 0);
        mzx_world->player_x = endgame_x;
        mzx_world->player_y = endgame_y;
      }
      else
      {
        mzx_world->target_board = endgame_board;
        mzx_world->target_where = TARGET_POSITION;
        mzx_world->target_x = endgame_x;
        mzx_world->target_y = endgame_y;
      }
      // Clear "endgame" part
      endgame_board = NO_ENDGAME_BOARD;
      // Give one more life with minimal health
      set_counter(mzx_world, "LIVES", 1, 0);
      set_counter(mzx_world, "HEALTH", 1, 0);
    }
    else
    {
      set_mesg(mzx_world, "Game over");
      src_board->b_mesg_row = 24;
      src_board->b_mesg_col = -1;
      if(mzx_world->game_over_sfx)
        play_sfx(mzx_world, 24);
      mzx_world->dead = 1;
    }
  }
  else

  if(get_counter(mzx_world, "HEALTH", 0) == 0)
  {
    int death_board = mzx_world->death_board;

    // Death
    set_counter(mzx_world, "HEALTH", mzx_world->starting_health, 0);
    dec_counter(mzx_world, "Lives", 1, 0);
    set_mesg(mzx_world, "You have died...");
    clear_sfx_queue();
    play_sfx(mzx_world, 23);

    // Go somewhere else?
    if(death_board != DEATH_SAME_POS)
    {
      if(death_board == NO_DEATH_BOARD)
      {
        int player_restart_x = mzx_world->player_restart_x;
        int player_restart_y = mzx_world->player_restart_y;

        if(player_restart_x >= board_width)
          player_restart_x = board_width - 1;

        if(player_restart_y >= board_height)
          player_restart_y = board_height - 1;

        // Return to entry x/y
        id_remove_top(mzx_world, mzx_world->player_x,
         mzx_world->player_y);
        id_place(mzx_world, player_restart_x, player_restart_y,
         PLAYER, 0, 0);
        mzx_world->player_x = player_restart_x;
        mzx_world->player_y = player_restart_y;
      }
      else
      {
        // Jump to given board
        if(mzx_world->current_board_id == death_board)
        {
          int death_x = mzx_world->death_x;
          int death_y = mzx_world->death_y;

          id_remove_top(mzx_world, mzx_world->player_x,
           mzx_world->player_y);
          id_place(mzx_world, death_x, death_y, PLAYER, 0, 0);
          mzx_world->player_x = death_x;
          mzx_world->player_y = death_y;
        }
        else
        {
          mzx_world->target_board = death_board;
          mzx_world->target_where = TARGET_POSITION;
          mzx_world->target_x = mzx_world->death_x;
          mzx_world->target_y = mzx_world->death_y;
        }
      }
    }
  }

  if(mzx_world->target_where != TARGET_TELEPORT)
  {
    int top_x, top_y;

    // Draw border
    draw_viewport(mzx_world);

    // Draw screen
    if(!game)
    {
      id_chars[player_color] = 0;
      id_chars[player_char + 0] = 32;
      id_chars[player_char + 1] = 32;
      id_chars[player_char + 2] = 32;
      id_chars[player_char + 3] = 32;
    }

    // Figure out x/y of top
    calculate_xytop(mzx_world, &top_x, &top_y);

    if(mzx_world->blind_dur > 0)
    {
      int i;
      int viewport_x = src_board->viewport_x;
      int viewport_y = src_board->viewport_y;
      int viewport_width = src_board->viewport_width;
      int viewport_height = src_board->viewport_height;
      int player_x = mzx_world->player_x;
      int player_y = mzx_world->player_y;

      for(i = viewport_y; i < viewport_y + viewport_height; i++)
      {
        fill_line(viewport_width, viewport_x, i, 176, 8);
      }

      // Find where player would be and draw.
      if(game)
      {
        id_put(src_board, player_x - top_x + viewport_x,
         player_y - top_y + viewport_y, player_x,
         player_y, player_x, player_y);
      }
    }
    else
    {
      draw_game_window(src_board, top_x, top_y);
    }

    // Add sprites
    draw_sprites(mzx_world);

    // Add time limit
    time_remaining = get_counter(mzx_world, "TIME", 0);
    if(time_remaining)
    {
      int edge_color = mzx_world->edge_color;
      int timer_color;
      if(edge_color == 15)
        timer_color = 0xF0; // Prevent white on white for timer
      else
        timer_color = (edge_color << 4) + 15;

      sprintf(tmp_str, "%d:%02d",
       time_remaining / 60, time_remaining % 60);
      write_string(tmp_str, 1, 24, timer_color, 0);

      // Border with spaces
      draw_char(' ', edge_color, strlen(tmp_str) + 1, 24);
      draw_char(' ', edge_color, 0, 24);
    }

    // Add message
    if(src_board->b_mesg_timer > 0)
    {
      int mesg_x = src_board->b_mesg_col;
      int mesg_y = src_board->b_mesg_row;

      if(mesg_y > 24)
        mesg_y = 24;

      char *bottom_mesg = src_board->bottom_mesg;
      int mesg_length = strlencolor(bottom_mesg);
      int mesg_edges = mzx_world->mesg_edges;

      if(mesg_x == -1)
        mesg_x = 40 - (mesg_length / 2);

      color_string_ext(bottom_mesg, mesg_x,
       mesg_y, scroll_color, 0, 0);

      if((mesg_x > 0) && (mesg_edges))
        draw_char(' ', scroll_color, mesg_x - 1, mesg_y);

      mesg_x += mesg_length;
      if((mesg_x < 80) && (mesg_edges))
        draw_char(' ', scroll_color, mesg_x, mesg_y);
    }

    // Add debug box

    if(debug_mode)
    {
      draw_debug_box(mzx_world, 60, 19, mzx_world->player_x,
       mzx_world->player_y);
    }

    if(update_music)
    {
      load_mod(src_board->mod_playing);
      strcpy(mzx_world->real_mod_playing, src_board->mod_playing);
    }

    update_music = 0;

    if(pal_update)
      update_palette();

    update_screen();
  }

  if(mzx_world->mzx_speed > 1)
  {
    // Number of ms the update cycle took
    total_ticks = (16 * (mzx_world->mzx_speed - 1))
     - (get_ticks() - start_ticks);
    if(total_ticks < 0)
      total_ticks = 0;
    // Delay for 16 * (speed - 1) since the beginning of the update
    delay(total_ticks);
  }

  if(*fadein)
  {
    vquick_fadein();
    *fadein = 0;
  }

  if(mzx_world->swapped)
  {
    src_board = mzx_world->current_board;
    load_mod(src_board->mod_playing);
    strcpy(mzx_world->real_mod_playing, src_board->mod_playing);

    send_robot_def(mzx_world, 0, 10);
    send_robot_def(mzx_world, 0, 11);

    return 1;
  }

  if(mzx_world->target_where != TARGET_NONE)
  {
    int saved_player_last_dir = src_board->player_last_dir;
    int target_board = mzx_world->target_board;

    // Aha.. TELEPORT or ENTRANCE.
    // Destroy message, bullets, spitfire?

    if(mzx_world->clear_on_exit)
    {
      int offset;
      mzx_thing d_id;

      src_board->b_mesg_timer = 0;
      for(offset = 0; offset < (board_width * board_height); offset++)
      {
        d_id = (mzx_thing)level_id[offset];
        if((d_id == SHOOTING_FIRE) || (d_id == BULLET))
          offs_remove_id(mzx_world, offset);
      }
    }

    // Load board
    mzx_world->under_player_id = (char)SPACE;
    mzx_world->under_player_param = 0;
    mzx_world->under_player_color = 7;

    mzx_world->current_board_id = target_board;
    mzx_world->current_board = mzx_world->board_list[target_board];
    src_board = mzx_world->current_board;

    if(strcasecmp(mzx_world->real_mod_playing, src_board->mod_playing) &&
     strcmp(src_board->mod_playing, "*"))
    {
      update_music = 1;
    }

    level_id = src_board->level_id;
    level_param = src_board->level_param;
    level_color = src_board->level_color;
    level_under_id = src_board->level_under_id;
    level_under_param = src_board->level_under_param;
    level_under_color = src_board->level_under_color;
    board_width = src_board->board_width;
    board_height = src_board->board_height;

    set_counter(mzx_world, "TIME", src_board->time_limit, 0);

    // Find target x/y
    if(mzx_world->target_where == TARGET_ENTRANCE)
    {
      int i;
      int tmp_x[5];
      int tmp_y[5];
      int x, y, offset;
      mzx_thing d_id;
      mzx_thing target_id = mzx_world->target_id;
      int target_color = mzx_world->target_color;

      // Entrance
      // First, set searched for id to the first in the whirlpool
      // series if it is part of the whirlpool series
      if(is_whirlpool(target_id))
        target_id = WHIRLPOOL_1;

      // Now scan. Order-
      // 1) Same type & color
      // 2) Same color
      // 3) Same type & foreground
      // 4) Same foreground
      // 5) Same type
      // Search for first of all 5 at once

      for(i = 0; i < 5; i++)
      {
        tmp_x[i] = -1; // None found
      }

      for(y = 0, offset = 0; y < board_height; y++)
      {
        for(x = 0; x < board_width; x++, offset++)
        {
          d_id = (mzx_thing)level_id[offset];

          if(d_id == PLAYER)
          {
            // Remove the player, maybe readd
            mzx_world->player_x = x;
            mzx_world->player_y = y;
            id_remove_top(mzx_world, x, y);
            // Grab again - might have revealed an entrance
            d_id = (mzx_thing)level_id[offset];
          }

          if(is_whirlpool(d_id))
            d_id = WHIRLPOOL_1;

          if(d_id == target_id)
          {
            // Same type
            // Color match?
            if(level_color[offset] == target_color)
            {
              // Yep
              tmp_x[0] = x;
              tmp_y[0] = y;
            }
            else

            // Fg?
            if((level_color[offset] & 0x0F) ==
             (target_color & 0x0F))
            {
              // Yep
              tmp_x[2] = x;
              tmp_y[2] = y;
            }
            else
            {
              // Just same type
              tmp_x[4] = x;
              tmp_y[4] = y;
            }
          }
          else

          if(flags[(int)d_id] & A_ENTRANCE)
          {
            // Not same type, but an entrance
            // Color match?
            if(level_color[offset] == target_color)
            {
              // Yep
              tmp_x[1] = x;
              tmp_y[1] = y;
            }
            // Fg?
            else

            if((level_color[offset] & 0x0F) == (target_color & 0x0F))
            {
              // Yep
              tmp_x[3] = x;
              tmp_y[3] = y;
            }
          }
          // Done with this x/y
        }
      }

      // We've got it... maybe.
      for(i = 0; i < 5; i++)
      {
        if(tmp_x[i] >= 0)
          break;
      }

      if(i < 5)
      {
        mzx_world->player_x = tmp_x[i];
        mzx_world->player_y = tmp_y[i];
      }

      id_place(mzx_world, mzx_world->player_x,
       mzx_world->player_y, PLAYER, 0, 0);
    }
    else
    {
      int target_x = mzx_world->target_x;
      int target_y = mzx_world->target_y;

      // Specified x/y
      if(target_x < 0)
        target_x = 0;

      if(target_y < 0)
        target_y = 0;

      if(target_x >= board_width)
        target_x = board_width - 1;

      if(target_y >= board_height)
        target_y = board_height - 1;

      find_player(mzx_world);
      place_player_xy(mzx_world, target_x, target_y);
    }

    send_robot_def(mzx_world, 0, 11);
    mzx_world->player_restart_x = mzx_world->player_x;
    mzx_world->player_restart_y = mzx_world->player_y;
    // Now... Set player_last_dir for direction FACED
    src_board->player_last_dir = (src_board->player_last_dir & 0x0F) |
     (saved_player_last_dir & 0xF0);

    // ...and if player ended up on ICE, set last dir pressed as well
    if((mzx_thing)level_under_id[mzx_world->player_x +
     (mzx_world->player_y * board_width)] == ICE)
    {
      src_board->player_last_dir = saved_player_last_dir;
    }

    // Fix palette
    if(mzx_world->target_where != TARGET_TELEPORT)
    {
      // Prepare for fadein
      if(!get_fade_status())
        *fadein = 1;
      vquick_fadeout();
    }

    mzx_world->target_where = TARGET_NONE;

    // Disallow any keypresses this cycle

    return 1;
  }

  return 0;
}

void find_player(World *mzx_world)
{
  Board *src_board = mzx_world->current_board;
  int board_width = src_board->board_width;
  int board_height = src_board->board_height;
  char *level_id = src_board->level_id;
  int dx, dy, offset;

  if(mzx_world->player_x >= board_width)
    mzx_world->player_x = 0;

  if(mzx_world->player_y >= board_height)
    mzx_world->player_y = 0;

  if((mzx_thing)level_id[mzx_world->player_x +
   (mzx_world->player_y * board_width)] != PLAYER)
  {
    for(dy = 0, offset = 0; dy < board_height; dy++)
    {
      for(dx = 0; dx < board_width; dx++, offset++)
      {
        if((mzx_thing)level_id[offset] == PLAYER)
        {
          mzx_world->player_x = dx;
          mzx_world->player_y = dy;
          return;
        }
      }
    }

    replace_player(mzx_world);
  }
}

int take_key(World *mzx_world, int color)
{
  int i;
  char *keys = mzx_world->keys;

  color &= 15;

  for(i = 0; i < NUM_KEYS; i++)
  {
    if(keys[i] == color) break;
  }

  if(i < NUM_KEYS)
  {
    keys[i] = NO_KEY;
    return 0;
  }

  return 1;
}

// Give a key. Returns non-0 if no room.
int give_key(World *mzx_world, int color)
{
  int i;
  char *keys = mzx_world->keys;

  color &= 15;

  for(i = 0; i < NUM_KEYS; i++)
    if(keys[i] == NO_KEY) break;

  if(i < NUM_KEYS)
  {
    keys[i] = color;
    return 0;
  }

  return 1;
}

void draw_debug_box(World *mzx_world, int x, int y, int d_x, int d_y)
{
  Board *src_board = mzx_world->current_board;
  int i;
  int robot_mem = 0;

  draw_window_box(x, y, x + 19, y + 5, EC_DEBUG_BOX, EC_DEBUG_BOX_DARK,
   EC_DEBUG_BOX_CORNER, 0, 1);

  write_string
  (
    "X/Y:        /     \n"
    "Board:            \n"
    "Robot mem:      kb\n",
    x + 1, y + 1, EC_DEBUG_LABEL, 0
  );

  write_number(d_x, EC_DEBUG_NUMBER, x + 8, y + 1, 5, 0, 10);
  write_number(d_y, EC_DEBUG_NUMBER, x + 14, y + 1, 5, 0, 10);
  write_number(mzx_world->current_board_id, EC_DEBUG_NUMBER,
   x + 18, y + 2, 0, 1, 10);

  for(i = 0; i < src_board->num_robots_active; i++)
  {
    robot_mem += (src_board->robot_list_name_sorted[i])->program_length;
  }

  write_number((robot_mem + 512) / 1024, EC_DEBUG_NUMBER,
   x + 12, y + 3, 5, 0, 10);

  if(*(src_board->mod_playing) != 0)
  {
    if(strlen(src_board->mod_playing) > 18)
    {
      char tempc = src_board->mod_playing[18];
      src_board->mod_playing[18] = 0;
      write_string(src_board->mod_playing, x + 1, y + 4,
       EC_DEBUG_NUMBER, 0);
      src_board->mod_playing[18] = tempc;
    }
    else
    {
      write_string(src_board->mod_playing, x + 1, y + 4,
       EC_DEBUG_NUMBER, 0);
    }
  }
  else
  {
    write_string("(no module)", x + 2, y + 4, EC_DEBUG_NUMBER, 0);
  }
}
