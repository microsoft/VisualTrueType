// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once 

#define END	(wchar_t)(-1)

class CommandLineOptions
{
public:
	CommandLineOptions(int argc, wchar_t **argv, const wchar_t *opts) : m_argc(argc), m_argv(argv), m_opts(opts), m_sp(1),
		m_optarg(nullptr), m_optind(1) {};
	~CommandLineOptions() {};

	wchar_t  GetOption()
	{
		wchar_t c;
		const wchar_t *cp;

		if (m_sp == 1)
			if (m_optind >= m_argc || m_argv[m_optind][0] != L'-' || m_argv[m_optind][1] == L'\0')
				return(END);
			else if (wcscmp(m_argv[m_optind], L"--") == 0) {
				m_optind++;
				return(END);
			}
			m_optopt = c = m_argv[m_optind][m_sp];
			if (c == L':' || (cp = wcschr(m_opts, c)) == nullptr) {
				if (m_argv[m_optind][++m_sp] == L'\0') {
					m_optind++;
					m_sp = 1;
				}
				return(L'?');
			}
			if (*++cp == L':') {
				if (m_argv[m_optind][m_sp + 1] != L'\0')
					m_optarg = &m_argv[m_optind++][m_sp + 1];
				else if (++m_optind >= m_argc) {
					m_sp = 1;
					return(L'?');
				}
				else
					m_optarg = m_argv[m_optind++];
				m_sp = 1;
			}
			else {
				if (m_argv[m_optind][++m_sp] == L'\0') {
					m_sp = 1;
					m_optind++;
				}
				m_optarg = nullptr;
			}
			return(c);
	}; 
	wchar_t  GetOptionOption() {return m_optopt;};
	wchar_t* GetOptionArgument() {return m_optarg;};
	int GetArgumentIndex() {return m_optind;};

private:
	int m_argc;
	int m_sp;
	int m_optind;
	wchar_t **m_argv;
	const wchar_t *m_opts;
	wchar_t *m_optarg;
	wchar_t m_optopt;
};


