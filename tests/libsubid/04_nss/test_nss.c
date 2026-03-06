#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <prototypes.h>
#include <stdbool.h>
#include <dlfcn.h>

extern bool nss_is_initialized();

extern struct subid_nss_db *get_subid_nss_db();

void check_files(struct subid_nss_db *h) {
	if (!h) {
		exit(1);
	}
	if (h->ops) {
		exit(1);
	}
}

void check_zzz(struct subid_nss_db *h) {
	if (!h) {
		exit(1);
	}
	if (!h->ops) {
		exit(1);
	}
}

void test1() {
	// nsswitch1 has no subid: entry
	setenv("LD_LIBRARY_PATH", ".", 1);
	printf("Test with no subid entry\n");
	nss_init("./nsswitch1.conf");
	if (!nss_is_initialized() || get_subid_nss_db())
		exit(1);
	// second run should change nothing
	printf("Test with no subid entry, second run\n");
	nss_init("./nsswitch1.conf");
	if (!nss_is_initialized() || get_subid_nss_db())
		exit(1);
}

void test2() {
	struct subid_nss_db *h;
	// nsswitch2 has a subid: files entry
	printf("test with 'files' subid entry\n");
	nss_init("./nsswitch2.conf");
	if (!nss_is_initialized())
		exit(1);
	h = get_subid_nss_db();
	check_files(h);
	if (h->next) {
		exit(1);
	}

	// second run should change nothing
	printf("test with 'files' subid entry, second run\n");
	nss_init("./nsswitch2.conf");
	if (!nss_is_initialized())
		exit(1);
	h = get_subid_nss_db();
	check_files(h);
	if (h->next) {
		exit(1);
	}
}

void test3() {
	struct subid_nss_db *h;
	// nsswitch3 has a subid: zzz entry
	printf("test with 'zzz' subid entry\n");
	nss_init("./nsswitch3.conf");
	if (!nss_is_initialized())
		exit(1);
	h = get_subid_nss_db();
	check_zzz(h);
	if (h->next)
		exit(1);

	// second run should change nothing
	printf("test with 'zzz' subid entry, second run\n");
	nss_init("./nsswitch3.conf");
	if (!nss_is_initialized())
		exit(1);
	h = get_subid_nss_db();
	check_zzz(h);
	if (h->next)
		exit(1);
}

void test4() {
	struct subid_nss_db *h;
	// nsswitch4 has a subid: files zzz
	printf("test with 'files zzz' subid entry\n");
	nss_init("./nsswitch4.conf");
	if (!nss_is_initialized())
		exit(1);
	h = get_subid_nss_db();
	check_files(h);
	if (!h->next)
		exit(1);
	check_zzz(h->next);
	if (h->next->next)
		exit(1);
}

void test5() {
	struct subid_nss_db *h;
	// nsswitch5 has a subid: zzz files
	printf("test with 'zzz files' subid entry\n");
	nss_init("./nsswitch5.conf");
	if (!nss_is_initialized())
		exit(1);
	h = get_subid_nss_db();
	check_zzz(h);
	if (!h->next)
		exit(1);
	check_files(h->next);
	if (h->next->next)
		exit(1);
}

const char *Prog;

int main(int argc, char *argv[])
{
	int which;

	Prog = Basename(argv[0]);

	if (argc < 1)
		exit(1);

	which = atoi(argv[1]);
	switch(which) {
	case 1: test1(); break;
	case 2: test2(); break;
	case 3: test3(); break;
	case 4: test4(); break;
	case 5: test5(); break;
	default: exit(1);
	}

	printf("nss parsing tests done\n");
	exit(0);
}
