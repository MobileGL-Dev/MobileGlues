// MobileGlues - version.h
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

#ifndef MOBILEGLUES_VERSION_H

#define VERSION_DEVELOPMENT 0
#define VERSION_ALPHA 1
#define VERSION_BETA 2
#define VERSION_RC 3
#define VERSION_RELEASE 10

#define MAJOR 2
#define MINOR 0
#define REVISION 1
#define PATCH 0

#define VERSION_TYPE VERSION_DEVELOPMENT

#if VERSION_TYPE == VERSION_RC
#define VERSION_RC_NUMBER 2
#endif

// Development builds are numbered for the reason release candidates are: several
// of them carry the same MAJOR.MINOR.REVISION, and a bug report has to be able to
// name which one it came from. Bump this whenever a build leaves this machine.
#if VERSION_TYPE == VERSION_DEVELOPMENT
#define VERSION_DEV_NUMBER 1
#endif

#define VERSION_SUFFIX ""

#define MOBILEGLUES_VERSION_H

#endif // MOBILEGLUES_VERSION_H
