#include <stdio.h>
#include <string.h>
#include "pcilib/build.h"

void BuildInfo() {
    printf("\n");
#ifdef PCILIB_RELEASE
    printf("Release: %s (revision: %s built on %s in %s)\n", PCILIB_RELEASE, PCILIB_REVISION, PCILIB_BUILD_DATE, PCILIB_BUILD_DIR);
#else /* PCILIB_RELEASE */
    printf("Revision: %s built on %s in %s\n", PCILIB_REVISION, PCILIB_BUILD_DATE, PCILIB_BUILD_DIR);
#endif /* PCILIB_RELEASE */
    printf("Branch: %s by %s\n", PCILIB_REVISION_BRANCH, PCILIB_REVISION_AUTHOR);
    if (strlen(PCILIB_REVISION_MODIFICATIONS)) {
	printf("Modifications: %s - %s\n",  PCILIB_LAST_MODIFICATION, PCILIB_REVISION_MODIFICATIONS);
    }
}
