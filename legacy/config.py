# -*- coding: utf-8 -*-
# https://lancellc.gitbook.io/clash/clash-config-file

import re
import yaml
import requests
import base64
import argparse
import sys
from time import strftime
from os import path

Profiles_ConnersHua = """
rule-providers:
    
  AdBlock:
    type: http
    behavior: classical
    path: ./RuleSet/AdBlock.yaml
    url: https://raw.githubusercontent.com/blackmatrix7/ios_rule_script/release/rule/Clash/Advertising/Advertising_Classical.yaml
    interval: 86400

  FSMEDIA:
    type: http
    behavior: classical
    path: ./RuleSet/FSMEDIA.yaml
    url: https://raw.githubusercontent.com/blackmatrix7/ios_rule_script/release/rule/Clash/GlobalMedia/GlobalMedia.yaml
    interval: 86400

  Global:
    type: http
    behavior: classical
    path: ./RuleSet/Global.yaml
    url: https://raw.githubusercontent.com/blackmatrix7/ios_rule_script/master/rule/Clash/Global/Global_Classical.yaml
    interval: 86400
    
  China:
    type: http
    behavior: classical
    path: ./RuleSet/China.yaml
    url: https://raw.githubusercontent.com/blackmatrix7/ios_rule_script/release/rule/Clash/China/China.yaml
    interval: 86400

  Telegram:
    type: http
    behavior: classical
    path: ./RuleSet/Telegram.yaml
    url: https://raw.githubusercontent.com/blackmatrix7/ios_rule_script/master/rule/Clash/Telegram/Telegram.yaml
    interval: 86400
    
  Microsoft:
    type: http
    behavior: classical
    path: ./RuleSet/Microsoft.yaml
    url: https://raw.githubusercontent.com/blackmatrix7/ios_rule_script/release/rule/Clash/Microsoft/Microsoft.yaml
    interval: 86400
    
  Steam:
    type: http
    behavior: classical
    path: ./RuleSet/Steam.yaml
    url: https://raw.githubusercontent.com/blackmatrix7/ios_rule_script/master/rule/Clash/Steam/Steam.yaml
    interval: 86400

# ALL POLICY YOU CAN USE: DIRECT, PROXY, DENIED, FSMEDIA, MATCH
rules:
  - RULE-SET,AdBlock,DENIED
  - RULE-SET,Telegram,PROXY
  - RULE-SET,Microsoft,PROXY
  - RULE-SET,Steam,STEAM
  - RULE-SET,FSMEDIA,FSMEDIA
  
  # GeoIP
  - GEOIP,CN,DIRECT,no-resolve
  
  - RULE-SET,Global,PROXY

  # https://raw.githubusercontent.com/blackmatrix7/ios_rule_script/master/rule/Clash/Lan/Lan.yaml,DIRECT,no-resolve
  - DOMAIN,instant.arubanetworks.com,DIRECT,no-resolve
  - DOMAIN,router.asus.com,DIRECT,no-resolve
  - DOMAIN,setmeup.arubanetworks.com,DIRECT,no-resolve
  - DOMAIN-SUFFIX,0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.ip6.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,0.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,1.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.ip6.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,10.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,100.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,100.51.198.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,101.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,102.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,103.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,104.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,105.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,106.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,107.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,108.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,109.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,110.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,111.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,112.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,113.0.203.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,113.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,114.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,115.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,116.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,117.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,118.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,119.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,120.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,121.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,122.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,123.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,124.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,125.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,126.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,127.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,127.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,16.172.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,168.192.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,17.172.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,18.172.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,19.172.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,2.0.192.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,20.172.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,21.172.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,22.172.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,23.172.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,24.172.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,25.172.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,254.169.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,255.255.255.255.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,26.172.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,27.172.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,28.172.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,29.172.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,30.172.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,31.172.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,64.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,65.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,66.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,67.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,68.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,69.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,70.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,71.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,72.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,73.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,74.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,75.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,76.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,77.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,78.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,79.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,8.b.d.0.1.0.0.2.ip6.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,8.e.f.ip6.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,80.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,81.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,82.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,83.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,84.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,85.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,86.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,87.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,88.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,89.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,9.e.f.ip6.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,90.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,91.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,92.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,93.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,94.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,95.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,96.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,97.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,98.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,99.100.in-addr.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,a.e.f.ip6.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,acl4.ssr,DIRECT,no-resolve
  - DOMAIN-SUFFIX,b.e.f.ip6.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,d.f.ip6.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,hiwifi.com,DIRECT,no-resolve
  - DOMAIN-SUFFIX,home.arpa,DIRECT,no-resolve
  - DOMAIN-SUFFIX,leike.cc,DIRECT,no-resolve
  - DOMAIN-SUFFIX,localhost.ptlogin2.qq.com,DIRECT,no-resolve
  - DOMAIN-SUFFIX,localhost.sec.qq.com,DIRECT,no-resolve
  - DOMAIN-SUFFIX,miwifi.com,DIRECT,no-resolve
  - DOMAIN-SUFFIX,msftconnecttest.com,DIRECT,no-resolve
  - DOMAIN-SUFFIX,msftncsi.com,DIRECT,no-resolve
  - DOMAIN-SUFFIX,my.router,DIRECT,no-resolve
  - DOMAIN-SUFFIX,p.to,DIRECT,no-resolve
  - DOMAIN-SUFFIX,peiluyou.com,DIRECT,no-resolve
  - DOMAIN-SUFFIX,phicomm.me,DIRECT,no-resolve
  - DOMAIN-SUFFIX,router.ctc,DIRECT,no-resolve
  - DOMAIN-SUFFIX,routerlogin.com,DIRECT,no-resolve
  - DOMAIN-SUFFIX,tendawifi.com,DIRECT,no-resolve
  - DOMAIN-SUFFIX,zte.home,DIRECT,no-resolve
  - IP-CIDR,0.0.0.0/8,DIRECT,no-resolve
  - IP-CIDR,10.0.0.0/8,DIRECT,no-resolve
  - IP-CIDR,100.64.0.0/10,DIRECT,no-resolve
  - IP-CIDR,127.0.0.0/8,DIRECT,no-resolve
  - IP-CIDR,169.254.0.0/16,DIRECT,no-resolve
  - IP-CIDR,172.16.0.0/12,DIRECT,no-resolve
  - IP-CIDR,192.0.0.0/24,DIRECT,no-resolve
  - IP-CIDR,192.0.2.0/24,DIRECT,no-resolve
  - IP-CIDR,192.168.0.0/16,DIRECT,no-resolve
  - IP-CIDR,192.88.99.0/24,DIRECT,no-resolve
  - IP-CIDR,198.18.0.0/15,DIRECT,no-resolve
  - IP-CIDR,198.51.100.0/24,DIRECT,no-resolve
  - IP-CIDR,203.0.113.0/24,DIRECT,no-resolve
  - IP-CIDR,224.0.0.0/3,DIRECT,no-resolve
  - IP-CIDR6,::1/128,DIRECT,no-resolve
  - IP-CIDR6,fc00::/7,DIRECT,no-resolve
  - IP-CIDR6,fe80::/10,DIRECT,no-resolve

  - RULE-SET,China,DIRECT

  - MATCH,MATCH
"""

if getattr(sys, 'frozen', False):
    # https://stackoverflow.com/a/404750/14168341
    CPATH = path.dirname(sys.executable)
else:
    CPATH = path.dirname(path.abspath(__file__))


def set_basic_info(
    *, port=7890, m_port=6789, s5_port=5678, log_lv="info", ext_port=9090
):
    return rf"""##############
# Generated at {strftime("%Y-%m-%d %H:%M:%S")}
##############
port: {port}
mixed-port: {m_port}
socks-port: {s5_port}
# redir-port: 7892
allow-lan: false
mode: rule # rule / global / direct
log-level: {log_lv} # silent / info / warning / error / debug
external-controller: '127.0.0.1:{ext_port}'
external-ui: ./dashboard # `http://{{external-controller}}/ui`
experimental:
  ignore-resolve-fail: true
dns:
  enable: true
  ipv6: false
  listen: '0.0.0.0:53'
  enhanced-mode: fake-ip # redir-host
  default-nameserver: # https://blog.skk.moe/post/which-public-dns-to-use/#公共-DNS-最佳实践
    - 119.28.28.28
    - 223.6.6.6
  nameserver-policy:
    'pic.ihcloud.net': '223.6.6.6'
  nameserver: # https://blog.skk.moe/post/alternate-surge-koolclash-as-gateway/#Clash-DNS Paragraph 2
    - https://doh.pub/dns-query
    - https://dns.alidns.com/dns-query
  fallback:
    - https://1.1.1.1/dns-query
    - 208.67.222.222 # OpenDNS
    - https://dns.google/dns-query
  fallback-filter:
    geoip: true
    geoip-code: CN
  fake-ip-range: 198.18.0.1/16
  fake-ip-filter: # 2022-1-8 更新
    # 以下域名列表参考自 vernesong/OpenClash 项目，并由 Hackl0us 整理补充
    # === LAN ===
    - '*.lan'
    # === Linksys Wireless Router ===
    - '*.linksys.com'
    - '*.linksyssmartwifi.com'
    # === Apple Software Update Service ===
    - 'swscan.apple.com'
    - 'mesu.apple.com'
    # === Windows 10 Connnect Detection ===
    - '*.msftconnecttest.com'
    - '*.msftncsi.com'
    # === NTP Service ===
    - 'time.*.com'
    - 'time.*.gov'
    - 'time.*.edu.cn'
    - 'time.*.apple.com'
    - 'time1.*.com'
    - 'time2.*.com'
    - 'time3.*.com'
    - 'time4.*.com'
    - 'time5.*.com'
    - 'time6.*.com'
    - 'time7.*.com'
    - 'ntp.*.com'
    - 'ntp.*.com'
    - 'ntp1.*.com'
    - 'ntp2.*.com'
    - 'ntp3.*.com'
    - 'ntp4.*.com'
    - 'ntp5.*.com'
    - 'ntp6.*.com'
    - 'ntp7.*.com'
    - '*.time.edu.cn'
    - '*.ntp.org.cn'
    - '+.pool.ntp.org'
    - 'time1.cloud.tencent.com'
    # === Music Service ===
    ## NetEase
    - '+.music.163.com'
    - '*.126.net'
    ## Baidu
    - 'musicapi.taihe.com'
    - 'music.taihe.com'
    ## Kugou
    - 'songsearch.kugou.com'
    - 'trackercdn.kugou.com'
    ## Kuwo
    - '*.kuwo.cn'
    ## JOOX
    - 'api-jooxtt.sanook.com'
    - 'api.joox.com'
    - 'joox.com'
    ## QQ
    - '+.y.qq.com'
    - '+.music.tc.qq.com'
    - 'aqqmusic.tc.qq.com'
    - '+.stream.qqmusic.qq.com'
    ## Xiami
    - '*.xiami.com'
    ## Migu
    - '+.music.migu.cn'
    # === Game Service ===
    ## Nintendo Switch
    - '+.srv.nintendo.net'
    ## Sony PlayStation
    - '+.stun.playstation.net'
    ## Microsoft Xbox
    - 'xbox.*.microsoft.com'
    - '+.xboxlive.com'
    # === Other ===
    ## QQ Quick Login
    - 'localhost.ptlogin2.qq.com'
    ## Golang
    - 'proxy.golang.org'
    ## STUN Server
    - 'stun.*.*'
    - 'stun.*.*.*'
    ## Bilibili CDN
    - '*.mcdn.bilivideo.cn'
"""


def deduplicate_list(*nodes):
    all_nodes = [node for each in nodes for node in each]
    all_names = {i['name'] for i in all_nodes}
    test = dict()
    for i, info in enumerate(all_nodes):
        n = info['name']
        if n not in test:
            test[n] = 1
        else:
            j = test[n]
            test[n] = j + 1
            while f'{n}_{str(j)}' in all_names:
                j += 1
            new_name = f'{n}_{str(j)}'
            test[new_name] = 1
            all_nodes[i]['name'] = new_name
            all_names.add(new_name)
    return all_nodes


def get_sub_list(*sub_url):
    subs = []
    requests.adapters.DEFAULT_RETRIES = 5
    # In case ProxyError: https://stackoverflow.com/a/28521696/14168341
    session = requests.Session()
    session.trust_env = False
    for i in sub_url:
        try:
            r = requests.get(i)
        except requests.exceptions.ProxyError as e:
            print(e)
            print(f'\nTry using session to bypass proxy...')
            r = session.get(i)
        sub = ""
        try:
            sub = decode_b64(r.text)
            print(sub)
        except:
            sub = parse_yaml(r.text)
        else:
            try:
                sub = parse_yaml(sub)
            except:
                sub = {'proxies': [i.strip()
                                   for i in sub.split('\n') if i.strip() != '']}
        if "proxies" in sub:
            subs.append(sub["proxies"])
        else:
            print(f'Subscription link\n{i}:\nNo support subscription yet.\n')
            # subs = parse_url(subs.strip().split("\n"))
    if len(subs):
        return deduplicate_list(*subs)
    else:
        return []


def make_proxy_group(subs):
    # if not subs:
    #     raise Exception('No subscription data avaliable.')
    region = ["JP", "HK", "TW", "US", "EA", "XX"]
    regex = ["日本", "深港|香港", "台湾|彰化", "美国", "韩国|新加坡|狮城"]
    group = [{"name": i, "type": "select", "proxies": []} for i in region]
    regex = [re.compile(i, re.I | re.A) for i in regex]
    # WARNING: If not use re.A, some CJK unicodes may be lost.
    for i in subs:
        for j, k in enumerate(regex):
            if k.search(i['name']):
                group[j]["proxies"].append(i['name'])
                break
        else:
            group[5]["proxies"].append(i['name'])
    group[0]["type"] = "fallback"
    group[0]["interval"] = 300
    group[0]["url"] = "http://connectivitycheck.gstatic.com/generate_204"
    # If array proxies' length is 0, Clash crash
    group = [g for g in group if len(g['proxies']) > 0]
    region = [g['name'] for g in group]
    group += [
        {"name": 'ALL', "type": "url-test", "interval": 7200, "tolerance": 20,
            "url": "http://youtube.com/generate_204", "proxies": [i['name'] for i in subs]},
        {"name": "PROXY", "type": "select",
            "proxies": ['ALL'] + region + ["DIRECT"]},
        {
            "name": "NATIVE",
            "type": "select",
            "proxies": ["DIRECT", "ALL"] + list(set(["HK", "TW", "JP"]) & set(region)),
        },
        # {"name": "CNMEDIA", "type": "select", "proxies": ["DIRECT", "HK", "TW"]},
        {
            "name": "FSMEDIA",
            "type": "select",
            "proxies": ["ALL"] + list(set(["HK", "TW", "JP", "US"]) & set(region)) + ["PROXY"],
        },
        {
            "name": "TIKTOK",
            "type": "select",
            "proxies": list(set(["JP", "US"]) & set(region)) + ["PROXY"],
        },
        {
            "name": "STEAM",
            "type": "select",
            "proxies": ["PROXY", "DIRECT"] + list(set(["XX", "HK", "TW", "JP", "US"]) & set(region)),
        },
        {"name": "MATCH", "type": "select",
            "proxies": ["PROXY", "DIRECT"]},
        {"name": "DENIED", "type": "select",
            "proxies": ["REJECT", "NATIVE"]},
    ]
    return dump_yaml('proxy-groups', group)


def add_custom_rules(rules):
    default = parse_yaml(Profiles_ConnersHua)
    return '\n\n'.join([
        yaml.dump({'rule-providers': default['rule-providers']}, indent=2),
        dump_yaml('rules', rules) + dump_yaml('rules',
                                              default['rules']).replace('rules:', '')
    ])


def dump_yaml(node_name, node_content):
    if not node_content:
        return f'{node_name}:\n\n'
    content = [
        "  - " + str(i).replace("'", '"').replace(" ", "") for i in node_content
    ]
    return f"{node_name}:\n" + "\n".join(content)


def parse_yaml(string):
    return yaml.load(string, Loader=yaml.SafeLoader)


def make_config(update_type, *sub_urls, **kw):
    body = [set_basic_info(), None, None, None]
    fp = path.join(CPATH, "config.yaml")
    if not path.exists(fp):
        update_type = 'all'
    elif update_type != 'all':
        with open(fp, 'r', encoding='utf-8') as f:
            config = parse_yaml(f.read())
            body[1] = dump_yaml('proxies', config.get('proxies'))
            body[2] = dump_yaml(
                'proxy-groups', config.get('proxy-groups'))
            body[3] = yaml.dump({'rule-providers': config.get('rule-providers')}, indent=2
                                ) + '\n\n' + dump_yaml('rules', config.get('rules'))
    if update_type != 'rules':
        subs = get_sub_list(*sub_urls)
        body[1] = dump_yaml('proxies', subs)
        body[2] = make_proxy_group(subs)
    if update_type != 'subs':
        body[3] = add_custom_rules(kw['rules'])
    with open(fp, 'w', encoding='utf-8') as f:
        f.write('\n\n'.join(body))
    print('Finished.')


def decode_b64(string):
    def p(s): return s + "=" * ((4 - len(s) % 4) % 4)
    return base64.urlsafe_b64decode(p(string)).decode("utf-8")


def safe_get(dic, key, ret=[]):
    val = dic.get(key, ret)
    if not val and val != ret:
        return ret
    else:
        return val


def main():
    parser = argparse.ArgumentParser(
        prog='MyClashConfigParser',
        description='Update or generate clash config info.\nIf params not given but needed, info will be tried to find in custom.yaml.\nIf no parameters given, all is used by default.')
    parser.add_argument(
        'pos', nargs='*', help="Update all info, as same as param '--all'.", metavar='urls')
    group = parser.add_mutually_exclusive_group()
    group.add_argument("-a", "--all", nargs='*', default=[],
                       help="Update subscriptions and rules.", metavar=('url', 'urls'))
    group.add_argument("-s", "--subs", nargs='*', default=[],
                       help="Just update subscriptions.", metavar=('url', 'urls'))
    group.add_argument("-r", "--rules", action='store_true',
                       help="Just add custom rules.")
    args = parser.parse_args()
    urls_arg, rules_arg = [], []
    fp = path.join(CPATH, "custom.yaml")
    if path.exists(fp):
        with open(fp, 'r', encoding='utf-8') as f:
            custom = parse_yaml(f.read())
            urls_arg = safe_get(custom, 'subs')
            rules_arg = safe_get(custom, 'rules')
    urls = args.pos + args.subs + args.all
    if args.rules:
        if not urls:
            update_type = 'rules'
        else:
            print("Error: Other parameters confilct with '-r/--rules'.")
            exit(0)
    elif args.subs:
        update_type = 'subs'
    else:
        update_type = 'all'
    if not urls_arg:
        urls_arg = urls
    make_config(update_type, *urls_arg, rules=rules_arg)


if __name__ == '__main__':
    main()
