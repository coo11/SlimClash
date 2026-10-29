from datetime import datetime, timezone


today = datetime.now(timezone.utc)
version = today.strftime("%y%m%d")
header = f"""#define VERSION_MAJOR {today.strftime("%y")}
#define VERSION_MONTH {today.month}
#define VERSION_DAY {today.day}
#define VERSION_STRING "{version}"
"""

with open("src/version.h", "w", encoding="ascii", newline="") as version_header:
    version_header.write(header)
print(version)
