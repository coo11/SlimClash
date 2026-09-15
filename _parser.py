# -*- coding: utf-8 -*-

from subprocess import run
from yaml import load, SafeLoader
from pathlib import Path


def extract_rules(fp):
    rules = []
    in_rules = False
    with open(fp, "r", encoding="utf-8") as f:
        for line in f:
            if not in_rules:
                if line.lstrip().startswith("rules:"):
                    in_rules = True
                continue
            if not line.strip():
                rules.append(line)
                continue
            stripped = line.lstrip()
            if stripped.startswith("-") or stripped.startswith("#"):
                rules.append(line)
                continue
            break
    return "".join(rules).strip()


def parse(arg):
    return load(arg, Loader=SafeLoader)


def download(url, filename):
    run(["curl", "-fsSL", "--create-dirs", "-o", filename, url], check=True)


def download_geo_data(basic):
    geo_dir = Path("geo_download")
    geo_dir.mkdir(exist_ok=True)
    for key, filename in (("geosite", "GeoSite.dat"), ("geoip", "GeoIP.dat"), ("asn", "ASN.mmdb"), ("mmdb", "geoip.metadb")):
        url = basic.get("geox-url", {}).get(key)
        if url:
            download(url, geo_dir / filename)


def download_ruleset(basic):
    ruleset_dir = Path("ruleset_download")
    ruleset_dir.mkdir(exist_ok=True)
    providers = basic.get("rule-providers", {})
    for i in providers.values():
        url = i.get("url")
        p = i.get("path")
        type = i.get("type")
        if type == "http" and url and p:
            download(url, ruleset_dir / p)


if __name__ == "__main__":
    print("Generating config.yaml....")
    basic = parse(Path("config/basic.yaml").read_text(encoding="utf-8"))
    download_geo_data(basic)
    download_ruleset(basic)
    with open("output.yaml", "w+") as f:
        f.write(Path("config/basic.yaml").read_text(encoding="utf-8").strip())
        f.write("\n\nrules:\n  ")
        f.write(extract_rules("config/custom-rules.yaml"))
        f.write("\n\n  ")
        f.write(extract_rules("config/rules.yaml"))
    print("Finished.")
