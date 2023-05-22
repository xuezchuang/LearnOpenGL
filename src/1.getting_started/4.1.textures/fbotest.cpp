#include <time.h>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include<soil.h>

#define WIN_W 640
#define WIN_H 480
#define WIN_X 0	// ☆
#define WIN_Y 0
#define CAPTURE 1
//#define IO 2

// PBO开关
#define PBO_TEST 1

GLuint  texName;
#if PBO_TEST
GLuint  pboName[2];
#endif

void add(unsigned char* src, int width, int height, int shift, unsigned char* dst);

// 自定义的方法
bool saveBMP(const char* lpFileName)
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	int width = viewport[2];
	int height = viewport[3];

	// 设置解包像素的对齐格式——Word对齐(4字节)
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	// 计算对齐后的真实宽度
	int nAlignWidth = (width * 24 + 31) / 32;

	#if PBO_TEST
	static int index = 0;
	index = (index + 1) % 2;
	int next_index = (index + 1) % 2;

	// PBO
	glReadBuffer(GL_FRONT);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pboName[index]);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, 0);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, pboName[next_index]);
	unsigned char* src = (unsigned char*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, nAlignWidth * height * 4, GL_MAP_READ_BIT);
	unsigned char* pdata = NULL;
	if (src)
	{
		pdata = new unsigned char[nAlignWidth * height * 4];
		memcpy(pdata, src, nAlignWidth * height * 4);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	}

	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	#else
	// 分配缓冲区
	unsigned char* pdata = new unsigned char[nAlignWidth * height * 4];
	memset(pdata, 0, nAlignWidth * height * 4);
	//从当前绑定的 frame buffer 读取 pixels
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pdata);
	#endif
	// ============================================

	if (!pdata)
	{
		printf("pdata is NULL! \n");
		return FALSE;
	}

	unsigned char* dst = new unsigned char[nAlignWidth * height * 4];

	// 耗时操作
	add(pdata, nAlignWidth, height, 0, dst);

	#ifdef IO
	// 以下就是为了 BMP 格式 做准备
	//由RGB变BGR
	for (int i = 0; i < width * height * 3; i += 3)
	{
		unsigned char tmpRGB;
		tmpRGB = pdata[i];
		pdata[i] = pdata[i + 2];
		pdata[i + 2] = tmpRGB;
	}

	// 设置 BMP 的文件头
	BITMAPFILEHEADER Header;
	BITMAPINFOHEADER HeaderInfo;
	Header.bfType = 0x4D42;
	Header.bfReserved1 = 0;
	Header.bfReserved2 = 0;
	Header.bfOffBits = (DWORD)(
		sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
	Header.bfSize = (DWORD)(
		sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)
		+ nAlignWidth * height * 4);
	HeaderInfo.biSize = sizeof(BITMAPINFOHEADER);
	HeaderInfo.biWidth = width;
	HeaderInfo.biHeight = height;
	HeaderInfo.biPlanes = 1;
	HeaderInfo.biBitCount = 24;
	HeaderInfo.biCompression = 0;
	HeaderInfo.biSizeImage = 4 * nAlignWidth * height;
	HeaderInfo.biXPelsPerMeter = 0;
	HeaderInfo.biYPelsPerMeter = 0;
	HeaderInfo.biClrUsed = 0;
	HeaderInfo.biClrImportant = 0;


	// 写入字节数据
	FILE* pfile;
	if (!(pfile = fopen(lpFileName, "wb+")))
	{
		printf("保存图像失败!\n");
		return FALSE;
	}
	fwrite(&Header, 1, sizeof(BITMAPFILEHEADER), pfile);
	fwrite(&HeaderInfo, 1, sizeof(BITMAPINFOHEADER), pfile);
	fwrite(pdata, 1, HeaderInfo.biSizeImage, pfile);
	fclose(pfile);
	#endif

	#ifndef PBO_TEST
	delete[] pdata;
	#else
	pdata = NULL;
	#endif
	delete[] dst;

	return TRUE;
}

GLuint loadGLTexture(const char* file_path)
{
	// 切记要初始化 OpenGL Context 和 GLEW ！
	/* load an image file directly as a new OpenGL texture */
	GLuint  tex_2d = SOIL_load_OGL_texture
	(
		file_path,
		SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID,
		SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
	);

	/* check for an error during the load process */
	if (0 == tex_2d)
	{
		printf("SOIL loading error: '%s'\n", SOIL_last_result());
	}

	return tex_2d;
}

void init(void)
{
	// ------------------------------------
	clock_t start = clock();
	// 比较耗时
	GLenum err = glewInit();
	if (glewIsSupported("GL_VERSION_2_0"))
		printf("Ready for OpenGL 2.0\n");
	else
	{
		printf("OpenGL 2.0 not supported\n");
		exit(1);
	}
	clock_t end = clock();
	double sec = double(end - start) / CLOCKS_PER_SEC;
	printf("glewInit: %fs\n", sec);
	// ------------------------------------

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glShadeModel(GL_FLAT);
	glEnable(GL_DEPTH_TEST);

	// ------------------------------------
	texName = loadGLTexture("../micky.png");

	// 按字节对齐
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// 区别1
	//glGenTextures(1, &texName);
	glBindTexture(GL_TEXTURE_2D, texName);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// 区别2
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageRec->sizeX, imageRec->sizeX,
	//	0, GL_RGB, GL_UNSIGNED_BYTE, imageRec->data);

	// ☆
	#if PBO_TEST
	glGenBuffers(2, pboName);

	int nAlignWidth = (WIN_W * 24 + 31) / 32;
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pboName[0]);
	glBufferData(GL_PIXEL_PACK_BUFFER, nAlignWidth * WIN_H * 4, NULL, GL_DYNAMIC_READ);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, pboName[1]);
	glBufferData(GL_PIXEL_PACK_BUFFER, nAlignWidth * WIN_H * 4, NULL, GL_DYNAMIC_READ);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	#endif
}

// 居然要 200+ms
void saveScreenShot(const char* img_path, int x, int y, int width, int height)
{
	int save_result = SOIL_save_screenshot
	(
		img_path,
		SOIL_SAVE_TYPE_BMP,
		x, y, width, height
	);

}

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

	glBindTexture(GL_TEXTURE_2D, texName);

	glBegin(GL_QUADS);
	// 左面
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-2.0, -1.0, 0.0);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-2.0, 1.0, 0.0);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(0.0, 1.0, 0.0);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(0.0, -1.0, 0.0);
	// 右面
	glTexCoord2f(0.0, 0.0);
	glVertex3f(1.0, -1.0, 0.0);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(1.0, 1.0, 0.0);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(2.41421, 1.0, -1.41421);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(2.41421, -1.0, -1.41421);
	glEnd();

	#if CAPTURE

	static int count = 1;

	// 根据时间来命名位图
	clock_t time = clock();
	char img_path[100];
	sprintf(img_path, "../bmp/%ld.bmp", time);

	saveBMP(img_path);
	//saveScreenShot(img_path, WIN_X, WIN_Y, WIN_W, WIN_H);	// 太慢

	clock_t elapse = clock() - time;
	//printf("elapse = %dms\n", elapse);

	static long sum = 0.;
	sum += elapse;
	float avg = 1. * sum / count;
	count++;

	printf("average = %fms\n", avg);

	# endif

	glFlush();
	glBindTexture(GL_TEXTURE_2D, 0);
}

//void reshape(int w, int h)
//{
//	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
//	glMatrixMode(GL_PROJECTION);
//	glLoadIdentity();
//	gluPerspective(60.0, (GLfloat)w / (GLfloat)h, 1.0, 30.0);	// ☆
//	glMatrixMode(GL_MODELVIEW);
//	glLoadIdentity();
//	glTranslatef(0.0, 0.0, -3.6);
//}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

// 测试：耗时操作
void add(unsigned char* src, int width, int height, int shift, unsigned char* dst)
{
	if (!src || !dst)
		return;

	int value;
	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j)
		{
			//printf("(%d, %d)\n", i, j);
			value = *src + shift;
			if (value > 255)
				*dst = (unsigned char)255;
			else
				*dst = (unsigned char)value;
			++src;
			++dst;

			value = *src + shift;
			if (value > 255)
				*dst = (unsigned char)255;
			else
				*dst = (unsigned char)value;
			++src;
			++dst;

			value = *src + shift;
			if (value > 255)
				*dst = (unsigned char)255;
			else
				*dst = (unsigned char)value;
			++src;
			++dst;

			++src;    // skip alpha
			++dst;
		}
	}
}

//#define DSA
#ifdef DSA
bool bUseDSA = true;
#else
bool bUseDSA = false;
#endif

int main()
{
	glfwInit();
	if (bUseDSA)
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	}
	else
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	}
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// build and compile our shader zprogram
	// ------------------------------------
	Shader ourShader("4.1.texture.vs", "4.1.texture.fs");

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float vertices[] = {
		// positions          // colors           // texture coords
		 0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
		 0.5f,  0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
		 -0.5f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
		 -0.5f, -0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left 
		 //
		// 0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
		// 0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
		//-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
		//-0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left 

		//1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
		//1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
		//0.0f,   1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
		//0.0f,   0.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left 
	};
	unsigned int indices[] = {
		0, 1, 3, // first triangle
		1, 2, 3  // second triangle
	};
	unsigned int VBO, VAO, EBO;
	unsigned int quadVAO, quadVBO;
	float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
	// positions   // texCoords
	-0.5,  0.5,  0.0f, 1.0f,
	-0.5, -0.5,  0.0f, 0.0f,
	 0.5, -0.5,  1.0f, 0.0f,

	-0.5,  0.5,  0.0f, 1.0f,
	 0.5, -0.5,  1.0f, 0.0f,
	 0.5,  0.5,  1.0f, 1.0f
	};

	if (bUseDSA)
	{
		glCreateVertexArrays(1, &quadVAO);
		glCreateBuffers(1, &quadVBO);
		glNamedBufferData(quadVBO, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
		glBindVertexArray(quadVAO);
		glVertexArrayVertexBuffer(quadVAO, 0, quadVBO, 0, 4 * sizeof(float));
		glVertexArrayAttribFormat(quadVAO, 0, 2, GL_FLOAT, GL_FALSE, 0);
		glVertexArrayAttribBinding(quadVAO, 0, 0);
		glEnableVertexArrayAttrib(quadVAO, 0);
		glVertexArrayAttribFormat(quadVAO, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
		glVertexArrayAttribBinding(quadVAO, 1, 0);
		glEnableVertexArrayAttrib(quadVAO, 1);
		glBindVertexArray(quadVAO);
		// 
		//glCreateVertexArrays(1, &VAO);
		//glCreateBuffers(1, &VBO);
		//glCreateBuffers(1, &EBO);
		//glBindVertexArray(VAO);

		////glBindBuffer(GL_ARRAY_BUFFER, VBO);
		//glNamedBufferData(VBO, sizeof(vertices), vertices, GL_STATIC_DRAW);
		////glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		//glNamedBufferData(EBO, sizeof(indices), indices, GL_STATIC_DRAW);

		//// 将顶点缓冲区绑定到顶点数组对象的一个绑定点上
		//glVertexArrayVertexBuffer(VAO, 0, VBO, 0, sizeof(float) * 8);

		//// 设置顶点属性0
		//glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
		//glVertexArrayAttribBinding(VAO, 0, 0);
		//glEnableVertexArrayAttrib(VAO, 0);

		//// 设置顶点属性1
		//glVertexArrayAttribFormat(VAO, 1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3);
		//glVertexArrayAttribBinding(VAO, 1, 0);
		//glEnableVertexArrayAttrib(VAO, 1);

		//// 设置顶点属性2
		//glVertexArrayAttribFormat(VAO, 2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 6);
		//glVertexArrayAttribBinding(VAO, 2, 0);
		//glEnableVertexArrayAttrib(VAO, 2);
		//glBindVertexArray(VAO);
	}
	else
	{
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		//glGenBuffers(1, &EBO);
		glBindVertexArray(quadVAO);

		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		// position attribute
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		// color attribute
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);
	}



	int width, height, nrChannels;
	unsigned int texture = loadTexture(FileSystem::getPath("resources/textures/grass.png").c_str());
	//unsigned int texture = loadTexture(FileSystem::getPath("resources/textures/container.jpg").c_str());

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// input
		// -----
		processInput(window);

		// render
		// ------
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// bind Texture
		if (bUseDSA)
		{
			glBindTextureUnit(0, texture);
			glActiveTexture(0);
		}
		else
		{
			glActiveTexture(0);
			glBindTexture(GL_TEXTURE_2D, texture);
		}

		// render container
		ourShader.use();
		glBindVertexArray(quadVAO);
		if (bUseDSA)
		{
			//glBindVertexArray(quadVAO);
			//glVertexArrayElementBuffer(VAO, EBO);
			glBindTextureUnit(0, texture);

			//glDrawArrays(GL_TRIANGLES, 0, 6);
			//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
		else
		{
			//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	glfwTerminate();
	return 0;
}



