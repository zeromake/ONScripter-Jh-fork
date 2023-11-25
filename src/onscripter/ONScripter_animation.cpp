/* -*- C++ -*-
 *
 *  ONScripter_animation.cpp - Methods to manipulate AnimationInfo
 *
 *  Copyright (c) 2001-2018 Ogapee. All rights reserved.
 *            (C) 2014-2018 jh10001 <jh10001@live.cn>
 *
 *  ogapee@aqua.dti2.ne.jp
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <SDL2_rotozoom.h>

#include "ONScripter.h"
#include "private/utils.h"
#ifdef USE_BUILTIN_LAYER_EFFECTS
#include "builtin_layer.h"
#endif

#define DEFAULT_CURSOR_WAIT ":l/3,160,2;cursor0.bmp"
#define DEFAULT_CURSOR_NEWPAGE ":l/3,160,2;cursor1.bmp"

int ONScripter::calcDurationToNextAnimation() {
    int min = 0;  // minimum next time

    for (int i = 0; i < 3; i++) {
        AnimationInfo *anim = &tachi_info[i];
        if (anim->visible && anim->is_animatable) {
            if (min == 0 || min > anim->next_time) min = anim->next_time;
        }
    }

    for (int i = MAX_SPRITE_NUM - 1; i >= 0; i--) {
        AnimationInfo *anim = &sprite_info[i];
        if (anim->visible && anim->is_animatable) {
            if (min == 0 || min > anim->next_time) min = anim->next_time;
        }
    }

    if (!textgosub_label &&
        (clickstr_state == CLICK_WAIT || clickstr_state == CLICK_NEWPAGE)) {
        AnimationInfo *anim = &cursor_info[0];  // CLICK_WAIT
        if (clickstr_state == CLICK_NEWPAGE) anim = &cursor_info[1];

        if (anim->visible && anim->is_animatable) {
            if (min == 0 || min > anim->next_time) min = anim->next_time;
        }
    }

#ifdef USE_LUA
    if (lua_handler.is_animatable && !script_h.isExternalScript()) {
        if (min == 0 || min > lua_handler.next_time)
            min = lua_handler.next_time;
    }
#endif

    return min;
}

void ONScripter::proceedAnimation(int current_time) {
    // 需要额外执行 sentence_font_info 的动画否则会导致 setwindow 的文本框图片无法显示（实际上是动画未执行导致一直在动画的第一帧刚好是隐藏）
    if (sentence_font_info.proceedAnimation(current_time))
        flushDirect(
            sentence_font_info.pos,
            refreshMode() | (draw_cursor_flag ? REFRESH_CURSOR_MODE : 0));
    for (int i = 0; i < 3; i++)
        if (tachi_info[i].proceedAnimation(current_time))
            flushDirect(
                tachi_info[i].pos,
                refreshMode() | (draw_cursor_flag ? REFRESH_CURSOR_MODE : 0));

    for (int i = MAX_SPRITE_NUM - 1; i >= 0; i--)
        if (sprite_info[i].proceedAnimation(current_time))
            flushDirect(
                sprite_info[i].pos,
                refreshMode() | (draw_cursor_flag ? REFRESH_CURSOR_MODE : 0));
    

#ifdef USE_LUA
    if (lua_handler.is_animatable && !script_h.isExternalScript()) {
        while (lua_handler.next_time <= current_time) {
            int tmp_event_mode = event_mode;
            int tmp_next_time = next_time;
            int tmp_string_buffer_offset = string_buffer_offset;

            char *current = script_h.getCurrent();
            if (lua_handler.isCallbackEnabled(LUAHandler::LUA_ANIMATION))
                if (lua_handler.callFunction(true, "animation"))
                    errorAndExit(lua_handler.error_str);
            script_h.setCurrent(current);
            readToken();

            string_buffer_offset = tmp_string_buffer_offset;
            next_time = tmp_next_time;
            event_mode = tmp_event_mode;

            lua_handler.next_time += lua_handler.duration_time;
            if (lua_handler.duration_time <= 0) {
                lua_handler.next_time = current_time;
                break;
            }
        }
    }
#endif

    if (!textgosub_label &&
        (clickstr_state == CLICK_WAIT || clickstr_state == CLICK_NEWPAGE)) {
        AnimationInfo *anim = &cursor_info[0];  // CLICK_WAIT
        if (clickstr_state == CLICK_NEWPAGE) anim = &cursor_info[1];

        if (anim->proceedAnimation(current_time)) {
            SDL_Rect dst_rect = anim->pos;
            if (!anim->abs_flag) {
                dst_rect.x += sentence_font.x() * screen_ratio1 / screen_ratio2;
                dst_rect.y += sentence_font.y() * screen_ratio1 / screen_ratio2;
            }
            flushDirect(
                dst_rect,
                refreshMode() | (draw_cursor_flag ? REFRESH_CURSOR_MODE : 0));
        }
    }
}

void ONScripter::setupAnimationInfo(AnimationInfo *anim, _FontInfo *info) {
    if (anim->trans_mode != AnimationInfo::TRANS_STRING && anim->file_name &&
        anim->surface_name &&
        strcmp(anim->file_name, anim->surface_name) == 0 &&
        ((!anim->mask_file_name && !anim->mask_surface_name) ||
         (anim->mask_file_name && !anim->mask_surface_name &&
          strcmp(anim->mask_file_name, anim->mask_surface_name) == 0)))
        return;

    anim->deleteSurface();
    anim->abs_flag = true;

#ifdef USE_BUILTIN_LAYER_EFFECTS
    if (anim->trans_mode != AnimationInfo::TRANS_LAYER)
#endif
    {
        anim->surface_name = new char[strlen(anim->file_name) + 1]{0};
        strcpy(anim->surface_name, anim->file_name);
    }

    if (anim->mask_file_name) {
        anim->mask_surface_name = new char[strlen(anim->mask_file_name) + 1]{0};
        strcpy(anim->mask_surface_name, anim->mask_file_name);
    }

    if (anim->trans_mode == AnimationInfo::TRANS_STRING) {
        _FontInfo f_info = sentence_font;
        if (info) f_info = *info;
        f_info.rubyon_flag = anim->is_ruby_drawable;
        f_info.types = ons_font::ANIM_FONT;

        if (anim->font_size_xy[0] >= 0) {  // in case of Sprite, not rclick menu
            f_info.setTateyokoMode(0);
            f_info.top_xy[0] = anim->orig_pos.x;
            f_info.top_xy[1] = anim->orig_pos.y;
            if (anim->is_single_line)
                f_info.setLineArea(strlen(anim->file_name) / 2 + 1);
            f_info.clear();

            f_info.font_size_xy[0] = anim->font_size_xy[0];
            f_info.font_size_xy[1] = anim->font_size_xy[1];
            f_info.pitch_xy[0] = anim->font_pitch[0];
            f_info.pitch_xy[1] = anim->font_pitch[1];

            f_info.ttf_font[0] = NULL;
            f_info.ttf_font[1] = NULL;
        }

        if (f_info.ttf_font[0] == NULL) {
            f_info.openFont(font_file,
                            screen_ratio1,
                            screen_ratio2,
                            generateFPath(),
                            getFontConfig(f_info.types));
        }

        SDL_Rect pos;
        if (anim->is_tight_region) {
            drawString(anim->file_name,
                       anim->color_list[anim->current_cell],
                       &f_info,
                       false,
                       NULL,
                       &pos);
        } else {
            f_info.savePoint();

            int xy[2] = {0, 0};
            f_info.setXY(f_info.num_xy[0] - 1, f_info.num_xy[1] - 1);
            pos = f_info.calcUpdatedArea(xy, screen_ratio1, screen_ratio2);

            f_info.rollback();
        }

        if (info != NULL) {
            info->copyPosition(&f_info);
        }

        anim->orig_pos.w = pos.w;
        anim->orig_pos.h = pos.h;
        anim->scalePosWH(screen_ratio1, screen_ratio2);
        anim->allocImage(
            anim->pos.w * anim->num_of_cells, anim->pos.h, texture_format);
        anim->fill(0, 0, 0, 0);

        f_info.top_xy[0] = f_info.top_xy[1] = 0;
        for (int i = 0; i < anim->num_of_cells; i++) {
            f_info.clear();
            drawString(anim->file_name,
                       anim->color_list[i],
                       &f_info,
                       false,
                       NULL,
                       NULL,
                       anim);
            f_info.top_xy[0] += anim->orig_pos.w;
        }
    }
#ifdef USE_BUILTIN_LAYER_EFFECTS
    else if (anim->trans_mode == AnimationInfo::TRANS_LAYER) {
        anim->allocImage(anim->pos.w, anim->pos.h, texture_format);
        anim->fill(0, 0, 0, 0);
    }
#endif
    else {
        bool has_alpha;
        int location;
        SDL_Surface *_surface1 = nullptr;
        SDL_Surface *_surface2 = nullptr;
        bool has_rescale = anim->load_size == NULL &&
            screen_ratio2 != screen_ratio1 &&
            (!disable_rescale_flag || location == BaseReader::ARCHIVE_TYPE_NONE);
        SDL_Point temp_size;
        SDL_Point *load_size = anim->load_size;
        if (has_rescale && (anim->orig_pos.w > 0 || anim->orig_pos.h > 0)) {
            temp_size.x = utils::min(anim->orig_pos.w, screen_width);
            temp_size.y = utils::min(anim->orig_pos.h, screen_height);
            load_size = &temp_size;
        }
        bool image_can_rescale = load_size == NULL && (*anim->file_name == '>' || strstr((char *)anim->file_name, ".svg"));
        _surface1 = loadImage(
            anim->file_name,
            &has_alpha,
            &location,
            &anim->default_alpha,
            load_size);
        // 通过新的大小重新 load 一次
        if (image_can_rescale && has_rescale) {
            int w, h;
            if ((w = _surface1->w * screen_ratio1 / screen_ratio2) == 0) w = 1;
            if ((h = _surface1->h * screen_ratio1 / screen_ratio2) == 0) h = 1;
            temp_size.x = w;
            temp_size.y = h;
            load_size = &temp_size;
            _surface1 = loadImage(
                anim->file_name,
                &has_alpha,
                &location,
                &anim->default_alpha,
                load_size);
        }
        if (anim->trans_mode == AnimationInfo::TRANS_MASK)
            _surface2 = loadImage(anim->mask_file_name);

        // SDL_Surface *surface = _surface1;
        SDL_Surface *surface = 
            anim->setupImageAlpha(_surface1, _surface2, has_alpha);


        bool is_rescaled = load_size &&
            ((load_size->x > 0 && surface->w == load_size->x) ||
            (load_size->y > 0 && surface->h == load_size->y));
        if (surface && has_rescale && !is_rescaled) {
            SDL_Surface *src_s = surface;

            int w, h;
            if ((w = src_s->w * screen_ratio1 / screen_ratio2) == 0) w = 1;
            if ((h = src_s->h * screen_ratio1 / screen_ratio2) == 0) h = 1;
            SDL_PixelFormat *fmt = image_surface->format;
            surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                           w,
                                           h,
                                           fmt->BitsPerPixel,
                                           fmt->Rmask,
                                           fmt->Gmask,
                                           fmt->Bmask,
                                           fmt->Amask);
            // SDL_Rect dest_rect{0, 0, w, h};
            // SDL_UpperBlit(src_s, NULL, surface, &dest_rect);
            resizeSurface(src_s, surface);
            // rotozoomSurface 性能非常的差……，甚至没有 ons 写的 resizeSurface
            // 好 surface = ::rotozoomSurface(
            //     src_s, 0, ((double)screen_ratio1 / (double)screen_ratio2),
            //     SMOOTHING_ON);
            SDL_FreeSurface(src_s);
        }

        anim->setImage(surface, texture_format);
        if (_surface2) SDL_FreeSurface(_surface2);
    }
}

void ONScripter::parseTaggedString(AnimationInfo *anim) {
    if (anim->image_name == NULL) return;
    anim->removeTag();

    int i;
    char *buffer = anim->image_name;
    anim->num_of_cells = 1;
    anim->trans_mode = trans_mode;

    if (buffer[0] == ':') {
        while (*++buffer == ' ')
            ;

        if (buffer[0] == 'a') {
            anim->trans_mode = AnimationInfo::TRANS_ALPHA;
            buffer++;
        } else if (buffer[0] == 'l') {
            anim->trans_mode = AnimationInfo::TRANS_TOPLEFT;
            buffer++;
        } else if (buffer[0] == 'r') {
            anim->trans_mode = AnimationInfo::TRANS_TOPRIGHT;
            buffer++;
        } else if (buffer[0] == 'c') {
            anim->trans_mode = AnimationInfo::TRANS_COPY;
            buffer++;
        } else if (buffer[0] == 's') {
            anim->trans_mode = AnimationInfo::TRANS_STRING;
            buffer++;
            anim->num_of_cells = 0;
            if (*buffer == '/') {
                buffer++;
                script_h.getNext();

                script_h.pushCurrent(buffer);
                anim->font_size_xy[0] = calcUserRatio(
                    calcFontSize(script_h.readInt(), ons_font::ANIM_FONT));
                anim->font_size_xy[1] = calcUserRatio(
                    calcFontSize(script_h.readInt(), ons_font::ANIM_FONT));
                anim->font_pitch[0] = anim->font_size_xy[0];
                anim->font_pitch[1] = anim->font_size_xy[0];  // dummy
                if (script_h.getEndStatus() & ScriptHandler::END_COMMA) {
                    int offset = calcUserRatio(script_h.readInt());
                    anim->font_pitch[0] += offset;
                    if (script_h.getEndStatus() & ScriptHandler::END_COMMA) {
                        script_h.readInt();  // 0 ... normal, 1 ... no
                                             // anti-aliasing, 2 ... Fukuro
                    }
                }
                buffer = script_h.getNext();
                script_h.popCurrent();
            } else {
                anim->font_size_xy[0] = sentence_font.font_size_xy[0];
                anim->font_size_xy[1] = sentence_font.font_size_xy[1];
                anim->font_pitch[0] = sentence_font.pitch_xy[0];
                anim->font_pitch[1] = sentence_font.pitch_xy[1];
            }

            while (buffer[0] != '#' && buffer[0] != '\0') buffer++;
            i = 0;
            while (buffer[i] == '#') {
                anim->num_of_cells++;
                i += 7;
            }
            anim->color_list = new uchar3[anim->num_of_cells];
            for (i = 0; i < anim->num_of_cells; i++) {
                readColor(&anim->color_list[i], buffer);
                buffer += 7;
            }
        } else if (buffer[0] == 'm') {
            anim->trans_mode = AnimationInfo::TRANS_MASK;
            char *start = ++buffer;
            while (buffer[0] != ';' && buffer[0] != '\n' && buffer[0] != '\0')
                buffer++;
            if (buffer[0] == ';')
                setStr(&anim->mask_file_name, start, buffer - start);
        } else if (buffer[0] == '#') {
            anim->trans_mode = AnimationInfo::TRANS_DIRECT;
            readColor(&anim->direct_color, buffer);
            buffer += 7;
        } else if (buffer[0] == '!') {
            anim->trans_mode = AnimationInfo::TRANS_PALLETTE;
            buffer++;
            anim->pallette_number = getNumberFromBuffer((const char **)&buffer);
        }

        if (anim->trans_mode != AnimationInfo::TRANS_STRING)
            while (buffer[0] != '/' && buffer[0] != ';' && buffer[0] != '\0')
                buffer++;
    }
#ifdef USE_BUILTIN_LAYER_EFFECTS
    else if (buffer[0] == '*') {
        anim->trans_mode = AnimationInfo::TRANS_LAYER;
        buffer++;
        anim->layer_no = getNumberFromBuffer((const char **)&buffer);
        LayerInfo *tmp = &layer_info[anim->layer_no];

        if (tmp->handler) {
            anim->pos.x = anim->pos.y = 0;
            anim->pos.w = screen_width;
            anim->pos.h = screen_height;
            tmp->handler->setSpriteInfo(sprite_info, anim);
            anim->duration_list = new int[1];
            anim->duration_list[0] = tmp->interval;
            anim->next_time = SDL_GetTicks() + tmp->interval;
            anim->is_animatable = true;
            utils::printInfo("setup a sprite for layer %d\n", anim->layer_no);
        } else
            anim->layer_no = -1;
        return;
    }
#endif

    if (buffer[0] == '/' && anim->trans_mode != AnimationInfo::TRANS_STRING) {
        buffer++;
        anim->num_of_cells = getNumberFromBuffer((const char **)&buffer);
        if (anim->num_of_cells == 0) {
            utils::printError(
                "ONScripter::parseTaggedString  The number of cells is 0\n");
            return;
        }

        anim->duration_list = new int[anim->num_of_cells];

        if (*buffer == ',') {
            buffer++;

            if (*buffer == '<') {
                buffer++;
                for (i = 0; i < anim->num_of_cells; i++) {
                    anim->duration_list[i] =
                        getNumberFromBuffer((const char **)&buffer);
                    buffer++;
                }
            } else {
                anim->duration_list[0] =
                    getNumberFromBuffer((const char **)&buffer);
                for (i = 1; i < anim->num_of_cells; i++)
                    anim->duration_list[i] = anim->duration_list[0];
            }
            anim->next_time = SDL_GetTicks() + anim->duration_list[0];

            buffer++;
            anim->loop_mode = *buffer++ - '0';  // 3...no animation
        } else {
            for (i = 0; i < anim->num_of_cells; i++) anim->duration_list[i] = 0;
            anim->loop_mode = 3;  // 3...no animation
        }
        if (anim->loop_mode != 3) anim->is_animatable = true;

        while (buffer[0] != ';' && buffer[0] != '\0') buffer++;
    }

    if (buffer[0] == ';' && anim->trans_mode != AnimationInfo::TRANS_STRING)
        buffer++;

    if (anim->trans_mode == AnimationInfo::TRANS_STRING && buffer[0] == '$') {
        script_h.pushCurrent(buffer);
        setStr(&anim->file_name, script_h.readStr());
        script_h.popCurrent();
    } else {
        setStr(&anim->file_name, buffer);
    }
}

void ONScripter::drawTaggedSurface(SDL_Surface *dst_surface,
                                   AnimationInfo *anim,
                                   SDL_Rect &clip) {
#ifdef USE_BUILTIN_LAYER_EFFECTS
    if (anim->trans_mode == AnimationInfo::TRANS_LAYER) {
        if (anim->layer_no >= 0) {
            LayerInfo *tmp = &layer_info[anim->layer_no];
            if (tmp->handler) tmp->handler->refresh(dst_surface, clip);
        }
        return;
    }
#endif
    SDL_Rect poly_rect = anim->pos;
    if (!anim->abs_flag) {
        poly_rect.x += sentence_font.x() * screen_ratio1 / screen_ratio2;
        poly_rect.y += sentence_font.y() * screen_ratio1 / screen_ratio2;
    }

    if (!anim->affine_flag)
        anim->blendOnSurface(
            dst_surface, poly_rect.x, poly_rect.y, clip, anim->trans);
    else
        anim->blendOnSurface2(
            dst_surface, poly_rect.x, poly_rect.y, clip, anim->trans);
}

void ONScripter::stopAnimation(int click) {
    int no;

    if (textgosub_label) return;

    if (click == CLICK_WAIT)
        no = 0;
    else if (click == CLICK_NEWPAGE)
        no = 1;
    else
        return;

    if (cursor_info[no].image_surface == NULL) return;

    SDL_Rect dst_rect = cursor_info[no].pos;

    if (!cursor_info[no].abs_flag) {
        dst_rect.x += sentence_font.x() * screen_ratio1 / screen_ratio2;
        dst_rect.y += sentence_font.y() * screen_ratio1 / screen_ratio2;
    }

    flushDirect(dst_rect, refreshMode());
}

void ONScripter::loadCursor(
    int no, const char *str, int x, int y, bool abs_flag) {
    AnimationInfo *ai = &cursor_info[no];

    if (str) {
        ai->setImageName(str);
    } else {
        if (no == 0)
            ai->setImageName(DEFAULT_CURSOR_WAIT);
        else
            ai->setImageName(DEFAULT_CURSOR_NEWPAGE);
    }
    ai->orig_pos.x = x;
    ai->orig_pos.y = y;
    ai->scalePosXY(screen_ratio1, screen_ratio2);

    parseTaggedString(ai);
    setupAnimationInfo(ai);

    if (filelog_flag)
        script_h.findAndAddLog(script_h.log_info[ScriptHandler::FILE_LOG],
                               ai->file_name,
                               true);  // a trick for save file
    ai->abs_flag = abs_flag;
    if (ai->image_surface)
        ai->visible = true;
    else
        ai->remove();

    if (str == NULL) {
        if (no == 0)
            ai->deleteImageName();
        else
            ai->deleteImageName();
    }
}
