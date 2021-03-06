
/*
 * WhatsApp API implementation in C++ for libpurple.
 * Written by David Guillen Fandos (david@davidgf.net) based 
 * on the sources of WhatsAPI PHP implementation.
 *
 * Share and enjoy!
 *
 */

#include "wa_api.h"
#include <cipher.h>
#include <vector>
#include <string>
#include <iostream>
#include <map>

class WhatsappConnection;

class Group {
public:
	Group(std::string id, std::string subject, std::string owner) {
		this->id = id;
		this->subject = subject;
		this->owner = owner;
	}
	~Group() {}
	std::string id,subject,owner;
	std::vector <std::string> participants;
};

class WhatsappConnectionAPI {
private:
	WhatsappConnection * connection;

public:
	WhatsappConnectionAPI(std::string phone, std::string password, std::string nick);
	~WhatsappConnectionAPI();
	
	void doLogin();
	void receiveCallback(const char * data, int len);
	int  sendCallback(char * data, int len);
	void sentCallback(int len);
	bool hasDataToSend();
	
	void addContacts(std::vector <std::string> clist);
	void sendChat(std::string to, std::string message);
	void sendGroupChat(std::string to, std::string message);
	bool query_chat(std::string & from, std::string & message,std::string & author);
	bool query_chatimages(std::string & from, std::string & preview, std::string & url);
	bool query_chatlocations(std::string & from, double & lat, double & lng, std::string & preview);
	bool query_chatsounds(std::string & from, std::string & url);
	bool query_status(std::string & from, int & stat);
	bool query_typing(std::string & from, int & status);
	bool query_icon(std::string & from, std::string & icon, std::string & hash);
	void account_info(unsigned long long & creation, unsigned long long & freeexp, std::string & status);
	void send_avatar(const std::string & avatar);
	int getuserstatus(const std::string & who);
	std::string getuserstatusstring(const std::string & who);
	unsigned long long getlastseen(const std::string & who);
	std::map <std::string,Group> getGroups();
	bool groupsUpdated();
	void leaveGroup(std::string group);
	void addGroup(std::string subject);
	void manageParticipant(std::string,std::string,std::string);
	
	void notifyTyping(std::string who, int status);
	void setMyPresence(std::string s, std::string msg);
	
	int loginStatus() const;

	int sendSSLCallback(char* buffer, int maxbytes);
	int sentSSLCallback(int bytessent);
	void receiveSSLCallback(char* buffer, int bytesrecv);
	bool hasSSLDataToSend();
	void SSLCloseCallback();
	bool hasSSLConnection(std::string & host, int * port);
};

char * waAPI_getgroups(void * waAPI) {
	std::map <std::string,Group> g = ((WhatsappConnectionAPI*)waAPI)->getGroups();
	std::string ids;
	for (std::map<std::string,Group>::iterator it = g.begin(); it != g.end(); it++) {
		if (it != g.begin()) ids += ",";
		ids += it->first;
	}
	return g_strdup(ids.c_str());
}

int waAPI_getgroupsupdated(void * waAPI) {
	if (((WhatsappConnectionAPI*)waAPI)->groupsUpdated()) return 1;
	return 0;
}

int waAPI_getgroupinfo(void * waAPI, char * id, char ** subject, char ** owner, char ** p) {
	std::map <std::string,Group> ret = ((WhatsappConnectionAPI*)waAPI)->getGroups();
	
	std::string sid = std::string(id);
	if (ret.find(sid) == ret.end()) return 0;
	
	std::string part;
	for (unsigned int i = 0; i < ret.at(sid).participants.size(); i++) {
		if (i != 0) part += ",";
		part += ret.at(sid).participants[i];
	}
	
	if (subject) *subject = g_strdup(ret.at(sid).subject.c_str());
	if (owner)   *owner = g_strdup(ret.at(sid).owner.c_str());
	if (p)       *p = g_strdup(part.c_str());
	
	return 1;
}
void waAPI_creategroup(void * waAPI, const char *subject) {
	((WhatsappConnectionAPI*)waAPI)->addGroup(std::string(subject));
}
void waAPI_deletegroup(void * waAPI, const char * subject) {
	((WhatsappConnectionAPI*)waAPI)->leaveGroup(std::string(subject));
}
void waAPI_manageparticipant(void * waAPI, const char *id, const char * part, const char * command) {
	((WhatsappConnectionAPI*)waAPI)->manageParticipant(std::string(id),std::string(part),std::string(command));
}

void waAPI_setavatar(void * waAPI, const void *buffer,int len) {
	std::string im((const char*)buffer,(size_t)len);
	((WhatsappConnectionAPI*)waAPI)->send_avatar(im);
}

int  waAPI_sendcb(void * waAPI, void * buffer, int maxbytes) {
	return ((WhatsappConnectionAPI*)waAPI)->sendCallback((char*)buffer,maxbytes);
}

void waAPI_senddone(void * waAPI, int bytessent) {
	return ((WhatsappConnectionAPI*)waAPI)->sentCallback(bytessent);
}

void waAPI_input(void * waAPI, const void * buffer, int bytesrecv) {
	((WhatsappConnectionAPI*)waAPI)->receiveCallback((char*)buffer,bytesrecv);
}

int  waAPI_hasoutdata(void * waAPI) {
	if (((WhatsappConnectionAPI*)waAPI)->hasDataToSend()) return 1;
	return 0;
}


int  waAPI_sslsendcb(void * waAPI, void * buffer, int maxbytes) {
	return ((WhatsappConnectionAPI*)waAPI)->sendSSLCallback((char*)buffer,maxbytes);
}

void waAPI_sslsenddone(void * waAPI, int bytessent) {
	((WhatsappConnectionAPI*)waAPI)->sentSSLCallback(bytessent);
}

void waAPI_sslinput(void * waAPI, const void * buffer, int bytesrecv) {
	((WhatsappConnectionAPI*)waAPI)->receiveSSLCallback((char*)buffer,bytesrecv);
}

int  waAPI_sslhasoutdata(void * waAPI) {
	if (((WhatsappConnectionAPI*)waAPI)->hasSSLDataToSend()) return 1;
	return 0;
}

int waAPI_hassslconnection(void * waAPI, char ** host, int * port) {
	std::string shost;
	bool r = ((WhatsappConnectionAPI*)waAPI)->hasSSLConnection(shost,port);
	if (r)
		*host = (char*)g_strdup(shost.c_str());
	return r;
}

void waAPI_sslcloseconnection(void * waAPI) {
	((WhatsappConnectionAPI*)waAPI)->SSLCloseCallback();
}



void waAPI_login(void * waAPI) {
	((WhatsappConnectionAPI*)waAPI)->doLogin();
}

void * waAPI_create(const char * username, const char * password, const char * nickname) {
	WhatsappConnectionAPI * api = new WhatsappConnectionAPI (username,password,nickname);
	return api;
}

void waAPI_delete(void * waAPI) {
	delete ((WhatsappConnectionAPI*)waAPI);
}

void waAPI_sendim(void * waAPI, const char * who, const char *message) {
	((WhatsappConnectionAPI*)waAPI)->sendChat(std::string(who),std::string(message));
}
void waAPI_sendchat(void * waAPI, const char * who, const char *message) {
	((WhatsappConnectionAPI*)waAPI)->sendGroupChat(std::string(who),std::string(message));
}

void waAPI_sendtyping(void * waAPI,const char * who,int typing) {
	((WhatsappConnectionAPI*)waAPI)->notifyTyping(std::string(who),typing);
}

int waAPI_querychat(void * waAPI, char ** who, char **message, char **author) {
	std::string f,m,a;
	if ( ((WhatsappConnectionAPI*)waAPI)->query_chat(f,m,a) ) {
		*who = g_strdup(f.c_str());
		*message = g_strdup(m.c_str());
		*author = g_strdup(a.c_str());
		return 1;
	}
	return 0;
}

void waAPI_accountinfo(void * waAPI, unsigned long long *creation, unsigned long long *freeexpires, char ** status) {
	std::string st;
	unsigned long long cr, fe;
	((WhatsappConnectionAPI*)waAPI)->account_info(cr,fe,st);
	*creation = cr;
	*freeexpires = fe;
	*status = g_strdup(st.c_str());
}

int waAPI_querychatimage(void * waAPI, char ** who, char **image, int * imglen, char ** url) {
	std::string fr,im,ur;
	if ( ((WhatsappConnectionAPI*)waAPI)->query_chatimages(fr,im,ur) ) {
		*who = g_strdup(fr.c_str());
		*image = (char*)g_memdup(im.c_str(),im.size());
		*imglen = im.size();
		*url = g_strdup(ur.c_str());
		return 1;
	}
	return 0;
}

int waAPI_querychatsound(void * waAPI, char ** who, char ** url) {
	std::string fr,ur;
	if ( ((WhatsappConnectionAPI*)waAPI)->query_chatsounds(fr,ur) ) {
		*who = g_strdup(fr.c_str());
		*url = g_strdup(ur.c_str());
		return 1;
	}
	return 0;
}

int waAPI_querychatlocation(void * waAPI, char ** who, char **image, int * imglen, double * lat, double * lng) {
	std::string fr,im;
	double la,ln;
	if ( ((WhatsappConnectionAPI*)waAPI)->query_chatlocations(fr,la,ln,im) ) {
		*who = g_strdup(fr.c_str());
		*lat = la;
		*lng = ln;
		*image = (char*)g_memdup(im.c_str(),im.size());
		*imglen = im.size();
		return 1;
	}
	return 0;
}

int waAPI_querystatus(void * waAPI, char ** who, int *stat) {
	std::string f; int st;
	if ( ((WhatsappConnectionAPI*)waAPI)->query_status(f,st) ) {
		*who = g_strdup(f.c_str());
		*stat = st;
		return 1;
	}
	return 0;
}
int waAPI_getuserstatus(void * waAPI, const char * who) {
	return ((WhatsappConnectionAPI*)waAPI)->getuserstatus(who);
}
char * waAPI_getuserstatusstring(void * waAPI, const char * who) {
	std::string s = ((WhatsappConnectionAPI*)waAPI)->getuserstatusstring(who);
	return g_strdup(s.c_str());
}
unsigned long long waAPI_getlastseen(void * waAPI, const char * who) {
	return ((WhatsappConnectionAPI*)waAPI)->getlastseen(who);
}

int waAPI_queryicon(void * waAPI, char ** who, char ** icon, int * len, char ** hash) {
	std::string f,ic,hs;
	if ( ((WhatsappConnectionAPI*)waAPI)->query_icon(f,ic,hs) ) {
		*who = g_strdup(f.c_str());
		*icon = (char*)g_memdup(ic.c_str(),ic.size());
		*len = ic.size();
		*hash = g_strdup(hs.c_str());
		return 1;
	}
	return 0;	
}

int waAPI_querytyping(void * waAPI, char ** who, int * stat) {
	std::string f; int status;
	if ( ((WhatsappConnectionAPI*)waAPI)->query_typing(f,status) ) {
		*who = g_strdup(f.c_str());
		*stat = status;
		return 1;
	}
	return 0;	
}

int waAPI_loginstatus(void * waAPI) {
	return ((WhatsappConnectionAPI*)waAPI)->loginStatus();
}

void waAPI_addcontact(void * waAPI, const char * phone) {
	std::vector <std::string> clist;
	clist.push_back(std::string(phone));
	((WhatsappConnectionAPI*)waAPI)->addContacts(clist);
}

void waAPI_delcontact(void * waAPI, const char * phone) {
	
}

void waAPI_setmypresence(void * waAPI, const char * st, const char * msg) {
	((WhatsappConnectionAPI*)waAPI)->setMyPresence(st,msg);
}

// Implementations when Openssl is not present

unsigned char * MD5(const unsigned char *d, int n, unsigned char *md) {
	PurpleCipher *md5_cipher;
	PurpleCipherContext *md5_ctx;
	
	md5_cipher = purple_ciphers_find_cipher("md5");
	md5_ctx = purple_cipher_context_new(md5_cipher, NULL);
	purple_cipher_context_append(md5_ctx, (guchar *)d, n);
	purple_cipher_context_digest(md5_ctx, 16, md, NULL);
	return md;
}

const char hmap[16]  = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
std::string tohex(const char * t, int l) {
	std::string ret;
	for (int i = 0; i < l; i++) {
		ret += hmap[((*t  )>>4)&0xF];
		ret += hmap[((*t++)   )&0xF];
	}
	return ret;
}

std::string md5hex(std::string target) {
	char outh[16];
	MD5((unsigned char*)target.c_str(), target.size(), (unsigned char*)outh);
	return tohex(outh,16);
}

std::string md5raw(std::string target) {
	char outh[16];
	MD5((unsigned char*)target.c_str(), target.size(), (unsigned char*)outh);
	return std::string(outh,16);
}

unsigned char * SHA1(const unsigned char *d, int n, unsigned char *md) {
	PurpleCipher *sha1_cipher;
	PurpleCipherContext *sha1_ctx;
	
	sha1_cipher = purple_ciphers_find_cipher("sha1");
	sha1_ctx = purple_cipher_context_new(sha1_cipher, NULL);
	purple_cipher_context_append(sha1_ctx, (guchar *)d, n);
	purple_cipher_context_digest(sha1_ctx, 20, md, NULL);
	return md;
}

int PKCS5_PBKDF2_HMAC_SHA1(const char *pass, int passlen, const unsigned char *salt, int saltlen, int iter,
                           int keylen, unsigned char *out) {
	unsigned char digtmp[20], *p, itmp[4];
	int cplen, j, k, tkeylen;
	int mdlen = 20;  // SHA1
	unsigned long i = 1;
	
	PurpleCipherContext *context = purple_cipher_context_new_by_name("hmac", NULL);
	
	p = out;
	tkeylen = keylen;
	while(tkeylen)
		{
		if(tkeylen > mdlen)
			cplen = mdlen;
		else
			cplen = tkeylen;
		/* We are unlikely to ever use more than 256 blocks (5120 bits!)
		 * but just in case...
		 */
		itmp[0] = (unsigned char)((i >> 24) & 0xff);
		itmp[1] = (unsigned char)((i >> 16) & 0xff);
		itmp[2] = (unsigned char)((i >> 8) & 0xff);
		itmp[3] = (unsigned char)(i & 0xff);
			
		purple_cipher_context_reset(context, NULL);
		purple_cipher_context_set_option(context, "hash", (gpointer)"sha1");
		purple_cipher_context_set_key_with_len(context, (guchar *)pass, passlen);
		purple_cipher_context_append(context, (guchar *)salt, saltlen);
		purple_cipher_context_append(context, (guchar *)itmp, 4);
		purple_cipher_context_digest(context, mdlen, digtmp, NULL);
		
		memcpy(p, digtmp, cplen);
		for(j = 1; j < iter; j++)
			{
			
			purple_cipher_context_reset(context, NULL);
			purple_cipher_context_set_option(context, "hash", (gpointer)"sha1");
			purple_cipher_context_set_key_with_len(context, (guchar *)pass, passlen);
			purple_cipher_context_append(context, (guchar *)digtmp, mdlen);
			purple_cipher_context_digest(context, mdlen, digtmp, NULL);

			for(k = 0; k < cplen; k++)
				p[k] ^= digtmp[k];
			}
		tkeylen-= cplen;
		i++;
		p+= cplen;
		}
	
	purple_cipher_context_destroy(context);
	
	return 1;
}


