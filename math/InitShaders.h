#pragma once

// Create a NULL-terminated string by reading the provided file
//  Helper function to load vertex and fragment shader files
inline char*
readShaderSource(const char* shaderFile)
{
	FILE* fp = fopen(shaderFile, "rb");

	if (fp == NULL) { return NULL; }

	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	char* buf = new char[size + 1];
	memset(buf, 0, size + 1);
	fread(buf, 1, size, fp);

	fclose(fp);

	return buf;
}

struct Shader {
	const char*  filename;
	GLenum       type;
	GLchar*      source;
};

// Create a GLSL program object from vertex and fragment shader files
inline GLuint
InitShader(const char* vShaderFile, const char* fShaderFile, const char* shared_file, vector<string>& preprocs, bool fail_means_die)
{
	Shader shaders[2] = {
			{ vShaderFile, GL_VERTEX_SHADER, NULL },
			{ fShaderFile, GL_FRAGMENT_SHADER, NULL }
	};

	const char* shared_file_text = readShaderSource(shared_file);
	if (!shared_file_text)
		exit(EXIT_FAILURE);

	string shared_file_text_s = shared_file_text;
	delete[] shared_file_text;

	GLuint program = glCreateProgram();

	for (int i = 0; i < 2; ++i) {
		Shader& s = shaders[i];
		s.source = readShaderSource(s.filename);
		if (shaders[i].source == NULL) {
			std::cerr << "Failed to read " << s.filename << std::endl;
			__debugbreak();
			if (fail_means_die)
				exit(EXIT_FAILURE);
		}

		string source;
		source += "#version 120\n";
		source += "#define TAU 6.28318530718\n";
		for (size_t k = 0; k < preprocs.size(); k++)
			source += "#define " + preprocs[k] + "\n";
		source += shared_file_text_s;
		source += "#line 0\n";
		source += s.source;
		const char* complete_source = source.c_str();

		GLuint shader = glCreateShader(s.type);
		glShaderSource(shader, 1, (const GLchar**)&complete_source, NULL);
		glCompileShader(shader);

		GLint  compiled;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (!compiled) {
			std::cerr << s.filename << " failed to compile:" << std::endl;
			GLint  logSize;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
			char* logMsg = new char[logSize];
			glGetShaderInfoLog(shader, logSize, NULL, logMsg);
			std::cerr << logMsg << std::endl;
			delete[] logMsg;

			__debugbreak();
			if (fail_means_die)
				exit(EXIT_FAILURE);
		}

		delete[] s.source;

		if (!compiled)
			return 0;

		glAttachShader(program, shader);
	}

	/* link  and error check */
	glLinkProgram(program);

	GLint  linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked) {
		std::cerr << "Shader program failed to link" << std::endl;
		GLint  logSize;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize);
		char* logMsg = new char[logSize];
		glGetProgramInfoLog(program, logSize, NULL, logMsg);
		std::cerr << logMsg << std::endl;
		delete[] logMsg;

		__debugbreak();
		if (fail_means_die)
			exit(EXIT_FAILURE);

		return 0;
	}

	/* use program object */
	glUseProgram(program);

	return program;
}
