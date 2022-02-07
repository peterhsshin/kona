/*
 * Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <vector>
#include <string>

#include "gl_common.h"

#define __CLASS__ "GLCommon"

namespace sdm {

GLuint GLCommon::LoadProgram(int vertex_entries, const char **vertex, int fragment_entries,
                           const char **fragment) {
  GLuint prog_id = glCreateProgram();

  int vert_id = glCreateShader(GL_VERTEX_SHADER);
  int frag_id = glCreateShader(GL_FRAGMENT_SHADER);

  GL(glShaderSource(vert_id, vertex_entries, vertex, 0));
  GL(glCompileShader(vert_id));
  DumpShaderLog(vert_id);

  GL(glShaderSource(frag_id, fragment_entries, fragment, 0));
  GL(glCompileShader(frag_id));
  DumpShaderLog(frag_id);

  GL(glAttachShader(prog_id, vert_id));
  GL(glAttachShader(prog_id, frag_id));

  GL(glLinkProgram(prog_id));

  GL(glDetachShader(prog_id, vert_id));
  GL(glDetachShader(prog_id, frag_id));

  GL(glDeleteShader(vert_id));
  GL(glDeleteShader(frag_id));

  return prog_id;
}

void GLCommon::DumpShaderLog(int shader) {
  int success = 0;
  GLchar infoLog[512];
  GL(glGetShaderiv(shader, GL_COMPILE_STATUS, &success));
  if (!success) {
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    DLOGI("Shader Failed to compile: %s\n", infoLog);
  }
}

void GLCommon::MakeCurrent(const GLContext* ctx) {
  DTRACE_SCOPED();
  EGL(eglMakeCurrent(ctx->egl_display, ctx->egl_surface, ctx->egl_surface, ctx->egl_context));
}

void GLCommon::SetProgram(uint32_t id) {
  DTRACE_SCOPED();
  GL(glUseProgram(id));
}

void GLCommon::DeleteProgram(uint32_t id) {
  DTRACE_SCOPED();
  GL(glDeleteProgram(id));
}

void GLCommon::SetSourceBuffer(const private_handle_t *src_hnd) {
  DTRACE_SCOPED();
  EGLImageBuffer *src_buffer = image_wrapper_.wrap(reinterpret_cast<const void *>(src_hnd));

  GL(glActiveTexture(GL_TEXTURE0));
  if (src_buffer) {
    GL(glBindTexture(GL_TEXTURE_2D, src_buffer->getTexture(GL_TEXTURE_2D)));
  }
}

void GLCommon::SetDestinationBuffer(const private_handle_t *dst_hnd) {
  DTRACE_SCOPED();
  EGLImageBuffer *dst_buffer = image_wrapper_.wrap(reinterpret_cast<const void *>(dst_hnd));

  if (dst_buffer) {
    GL(glBindFramebuffer(GL_FRAMEBUFFER, dst_buffer->getFramebuffer()));
  }
}

int GLCommon::WaitOnInputFence(const std::vector<int> &in_fence_fds) {
  DTRACE_SCOPED();

  int merged_fd = GetMergedFd(in_fence_fds, "Acquire_fence", true);
  if (merged_fd == -1) {
    return 0;
  }

  EGLint attribs[] = {EGL_SYNC_NATIVE_FENCE_FD_ANDROID, merged_fd, EGL_NONE};
  EGLSyncKHR sync = eglCreateSyncKHR(eglGetCurrentDisplay(), EGL_SYNC_NATIVE_FENCE_ANDROID,
                                     attribs);

  if (sync == EGL_NO_SYNC_KHR) {
    DLOGE("Failed to create sync from source fd: %d", merged_fd);
    return -1;
  } else {
    EGL(eglWaitSyncKHR(eglGetCurrentDisplay(), sync, 0));
    EGL(eglDestroySyncKHR(eglGetCurrentDisplay(), sync));
  }

  return 0;
}

int GLCommon::CreateOutputFence() {
  DTRACE_SCOPED();
  int fd = -1;
  EGLSyncKHR sync = eglCreateSyncKHR(eglGetCurrentDisplay(), EGL_SYNC_NATIVE_FENCE_ANDROID, NULL);

  // Swap buffer.
  GL(glFlush());

  if (sync == EGL_NO_SYNC_KHR) {
    DLOGE("Failed to create egl sync fence");
  } else {
    fd = eglDupNativeFenceFDANDROID(eglGetCurrentDisplay(), sync);
    if (fd == EGL_NO_NATIVE_FENCE_FD_ANDROID) {
      DLOGE("Failed to dup sync");
    }
    EGL(eglDestroySyncKHR(eglGetCurrentDisplay(), sync));
  }

  return fd;
}

void GLCommon::DestroyContext(GLContext* ctx) {
  DTRACE_SCOPED();

  // Clear egl image buffers.
  image_wrapper_.Deinit();

  EGL(DeleteProgram(ctx->program_id));
  EGL(eglMakeCurrent(ctx->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
  EGL(eglDestroySurface(ctx->egl_display, ctx->egl_surface));
  EGL(eglDestroyContext(ctx->egl_display, ctx->egl_context));
  EGL(eglTerminate(ctx->egl_display));
}

void GLCommon::ClearCache() {
  // Clear cached handles.
  image_wrapper_.Deinit();
  image_wrapper_.Init();
}

void GLCommon::SetRealTimePriority() {
  // same as composer thread
  struct sched_param param = {0};
  param.sched_priority = 2;
  if (sched_setscheduler(0, SCHED_FIFO | SCHED_RESET_ON_FORK, &param) != 0) {
    DLOGE("Couldn't set SCHED_FIFO: %d", errno);
  }
}

void GLCommon::SetViewport(const GLRect &dst_rect) {
  DTRACE_SCOPED();
  float width = dst_rect.right - dst_rect.left;
  float height = dst_rect.bottom - dst_rect.top;
  GL(glViewport(dst_rect.left, dst_rect.top, width, height));
}

int GLCommon::GetMergedFd(const std::vector<int> &fence_fds, const std::string &desc,
                          bool ignore_signaled) {
  DTRACE_SCOPED();
  int merged_fd = -1;
  for (auto &fence_fd : fence_fds) {
    if (fence_fd == -1) {
      continue;
    }

    // Fence got signaled.
    if (ignore_signaled && (sync_wait(fence_fd, 0) >= 0)) {
      continue;
    }

    if (merged_fd == -1) {
      merged_fd = dup(fence_fd);
    } else {
      int temp = merged_fd;
      merged_fd = sync_merge(desc.c_str(), fence_fd, merged_fd);
      close(temp);
    }
  }

  return merged_fd;
}

}  // namespace sdm

