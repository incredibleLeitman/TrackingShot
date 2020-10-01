#include "textureManager.h"

#include <string>
#include <iostream>

TextureManager* TextureManager::m_inst(0);

TextureManager* TextureManager::Inst() {
    if (!m_inst)
        m_inst = new TextureManager();

    return m_inst;
}

TextureManager::TextureManager() {
#ifdef FREEIMAGE_LIB
    FreeImage_Initialise(); // call this ONL-Y when linking with FreeImage as a static library
#endif
}

TextureManager::~TextureManager() {
#ifdef FREEIMAGE_LIB
    FreeImage_DeInitialise(); // call this ONLY when linking with FreeImage as a static library
#endif

    unloadAllTextures();
    m_inst = 0;
}

void TextureManager::reportGLError(const char* msg) {
    GLenum errCode;
    const GLubyte* errString;
    while ((errCode = glGetError()) != GL_NO_ERROR) {
        errString = gluErrorString(errCode);
        fprintf(stderr, "OpenGL Error: %s %s\n", msg, errString);
    }
    return;
}

bool
TextureManager::loadTexture(const char* filename, const unsigned int texID, GLint level, GLenum image_format, GLint internal_format, GLint border) {
    tgaInfo* info = 0;
    info = tgaLoad((char*)filename);
    if (info->status != TGA_OK) {
        fprintf(stderr, "error loading texture image: %s\n", filename);
        return false;
    }

    //if this texture ID is in use, unload the current texture
    if (m_texID.find(texID) != m_texID.end())
        glDeleteTextures(1, &(m_texID[texID]));

    GLuint gl_texID;
    int mode = info->pixelDepth / 8; // will be 3 for rgb, 4 for rgba
    GLint format = (mode == 4) ? GL_RGBA : GL_RGB;
    GLsizei w = info->width;
    GLsizei h = info->height;

    glGenTextures(1, &gl_texID); //generate an OpenGL texture ID for this texture
    m_texID[texID] = gl_texID; //store the texture ID mapping
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // should be default
    glBindTexture(GL_TEXTURE_2D, gl_texID); //bind to the new texture ID

    // additional params:
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (level > 0) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    }
    else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    GLint clamp_mode = GL_REPEAT; // or GL_CLAMP, GL_CLAMP_TO_EDGE in newer versions
    clamp_mode = GL_CLAMP;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp_mode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp_mode);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    reportGLError("before uploading texture");

    //store the texture data for OpenGL use
    //glTexImage2D(GL_TEXTURE_2D, level, internal_format, width, height, border, image_format, GL_UNSIGNED_BYTE, bits);
    if (level > 0) {
        //glGenerateMipmap(GL_TEXTURE_2D); // only available for OpenGL 3+
        std::string str(filename);
        std::size_t pos = str.find('.');
        str = str.substr(0, pos);

        for (int i = 0; i < level; i++, w /= 2, h /= 2) {
            std::string file = str + std::to_string(w) + ".tga";
            fprintf(stderr, "loading file: %s\n", file.c_str());
            info = tgaLoad(&file[0]);
            if (info->status != TGA_OK) {
                fprintf(stderr, "error loading texture image: %s\n", filename);
                return false;
            }
            glTexImage2D(GL_TEXTURE_2D, i, format, w, h, 0, format, GL_UNSIGNED_BYTE, info->imageData);
        }
    }
    // mipmapping
    else {
        glTexImage2D(GL_TEXTURE_2D, level, format, w, h, 0, format, GL_UNSIGNED_BYTE, info->imageData);
    }
    reportGLError("after uploading texture");

    tgaDestroy(info);
    return true;
}

bool TextureManager::unloadTexture(const unsigned int texID) {
    bool result(true);
    //if this texture ID mapped, unload it's texture, and remove it from the map
    if (m_texID.find(texID) != m_texID.end()) {
        glDeleteTextures(1, &(m_texID[texID]));
        m_texID.erase(texID);
    }
    //otherwise, unload failed
    else {
        result = false;
    }

    return result;
}

bool TextureManager::bindTexture(const unsigned int texID) {
    bool result(true);
    if (m_texID.find(texID) != m_texID.end()) //if this texture ID mapped, bind it's texture as current
    {
        glBindTexture(GL_TEXTURE_2D, m_texID[texID]);
    }
    else //otherwise, binding failed
    {
        fprintf(stderr, "error binding texture image for id: %d\n", texID);
        result = false;
    }
    return result;
}

void TextureManager::unloadAllTextures() {
    //start at the begginning of the texture map
    std::map<unsigned int, GLuint>::iterator i = m_texID.begin();

    //Unload the textures untill the end of the texture map is found
    while (i != m_texID.end())
        unloadTexture(i->first);

    //clear the texture map
    m_texID.clear();
}