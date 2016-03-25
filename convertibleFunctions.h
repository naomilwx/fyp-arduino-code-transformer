#ifndef _CONVERTIBLEFUNCTIONS_H_
#define _CONVERTIBLEFUNCTIONS_H_

#include <boost/assert.hpp>
#include <boost/assign/list_of.hpp>
#include <map>

//using namespace boost::assign;
namespace ConvertibleFunctions {
 typedef std::vector<int> param_pos_list;
 typedef std::pair< std::string, param_pos_list > func_details;
 typedef std::map<std::string, func_details> pfunc_map;

 const func_details STRCHRP = {"strchr_P", {0}};//returns char in pgmspace
 const func_details STRCHRNULP = {"strchrnul_P", {0}};//returns char in pgmspace
 const func_details STRRCHRP = {"strrchr_P", {0}}; //returns char in pgmspace

 const func_details STRCSPNP = {"strcspn_P", {1}}; //first argument must be a string in mem
 const func_details STRCMPP = {"strcmp_P", {1}}; //first argument must be a string in mem
 const func_details STRNCASECMPP = {"strncasecmp_P", {1}}; //first argument must be a string in mem
 const func_details STRPBRKP = {"strpbrk_P", {1}}; //first arg must be a string in memory
 const func_details STRSPNP = {"strspn_P", {1}}; //first arg must be a string in memory
 const func_details STRSTRP = {"strstr_P", {1}}; //first arg must be a string in memory

 const func_details STRNLENP = {"strnlen_P", {0}};
 const func_details STRLENP = {"strlen_P", {0}};
 const func_details STRCPYP = {"strcpy_P", {1}};
 const func_details STRCATP = {"strcat_P", {1}};
 const func_details STRLCATP = {"strlcat_P", {1}};
 const func_details STRLCPYP = {"strlcpy_P", {1}};
 const func_details STRNCATP = {"strncat_P", {1}};
 const func_details STRNCPYP = {"strncpy_P", {1}};
 const func_details STRSEPP = {"strsep_P", {1}};
 const func_details SPRINTFP = {"sprintf_P", {1}};

 const pfunc_map PFUNCTION_MAP = boost::assign::list_of<std::pair<std::string, func_details>>
		 	 	 	 	 	 ("strcpy", STRCPYP)
							 ("strcat", STRCATP)
							 ("sprintf", SPRINTFP)
							 ("strcmp", STRCMPP)
							 ("strstr", STRSTRP)
							 ("strlen", STRLENP);
							 
}

#endif
