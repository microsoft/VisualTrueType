// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once 
#include <cstring>

#define END	(char)(-1)

class CommandLineOptions
{
public:
	CommandLineOptions(int argc, char **argv, const char *opts) : m_argc(argc), m_argv(argv), m_opts(opts), m_sp(1),
		m_optarg(nullptr), m_optind(1) {};
	~CommandLineOptions() {};

	char GetOption()
	{
		char c;
		const char *cp;

		if (m_sp == 1) {
			if (m_optind >= m_argc || m_argv[m_optind][0] != '-' || m_argv[m_optind][1] == '\0')
				return(END);
			else if (strcmp(m_argv[m_optind], "--") == 0) {
				m_optind++;
				return(END);
			}
		}
		m_optopt = c = m_argv[m_optind][m_sp];
		if (c == ':' || (cp = strchr(m_opts, c)) == nullptr) {
			if (m_argv[m_optind][++m_sp] == '\0') {
				m_optind++;
				m_sp = 1;
			}
			return('?');
		}
		if (*++cp == ':') {
			if (m_argv[m_optind][m_sp + 1] != '\0')
				m_optarg = &m_argv[m_optind++][m_sp + 1];
			else if (++m_optind >= m_argc) {
				m_sp = 1;
				return('?');
			}
			else
				m_optarg = m_argv[m_optind++];
			m_sp = 1;
		}
		else {
			if (m_argv[m_optind][++m_sp] == '\0') {
				m_sp = 1;
				m_optind++;
			}
			m_optarg = nullptr;
		}
		return(c);
	}; 
	char  GetOptionOption() {return m_optopt;};
	char* GetOptionArgument() {return m_optarg;};
	int GetArgumentIndex() {return m_optind;};

private:
	int m_argc;
	int m_sp;
	int m_optind;
	char **m_argv;
	const char *m_opts;
	char *m_optarg;
	char m_optopt;
};


