"""Small stdlib-only LAN integration client. No GUI or sensor dependency."""
from __future__ import annotations
import argparse
import json
import os
from urllib.request import Request,ProxyHandler,HTTPRedirectHandler,build_opener
from discovery import normalize_host
from desk_notification import notification_payload
from integration_feed import validate_feed


class NoRedirect(HTTPRedirectHandler):
    def redirect_request(self,*args,**kwargs):return None


class IntegrationClient:
    def __init__(self,host,key,timeout=2.0,port=80):
        self.host=normalize_host(host)
        if type(port) is not int or not 1<=port<=65535:raise ValueError("Invalid HTTP port")
        self.port=port
        if not isinstance(key,str) or not key:raise ValueError("Pairing key is required")
        self.key=key;self.timeout=timeout;self.opener=build_opener(ProxyHandler({}),NoRedirect())

    def _post(self,path,payload):
        body=json.dumps(payload,separators=(',',':')).encode('utf-8')
        request=Request(f'http://{self.host}:{self.port}{path}',data=body,headers={'Content-Type':'application/json','X-MEO-KEY':self.key})
        with self.opener.open(request,timeout=self.timeout) as response:
            raw=response.read(8193)
        if len(raw)>8192:raise ValueError("Device response exceeds 8 KiB")
        result=json.loads(raw)
        if not isinstance(result,dict) or result.get('ok') is not True:raise ValueError("Device rejected integration payload")
        return result

    def feed(self,slot,value,name='Data',unit='',ttl=30):
        if type(slot) is not int or not 0<=slot<3:raise ValueError("Feed slot must be 0, 1 or 2")
        return self._post(f'/api/v1/feeds/{slot}',validate_feed(dict(name=name,value=value,unit=unit,ttl_s=ttl)))

    def notify(self,text='New notification',hidden=True,source='integration'):
        return self._post('/api/v1/notifications',notification_payload(text,hidden,source))


def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host',required=True);parser.add_argument('--key-env',default='MEO_API_KEY')
    parser.add_argument('--port',type=int,default=80)
    parser.add_argument('--dry-run',action='store_true',help='Print payload only; never connect')
    commands=parser.add_subparsers(dest='command',required=True)
    feed=commands.add_parser('feed');feed.add_argument('--slot',type=int,choices=range(3),required=True)
    feed.add_argument('--name',default='Data');feed.add_argument('--value',required=True);feed.add_argument('--unit',default='');feed.add_argument('--ttl',type=int,default=30)
    notify=commands.add_parser('notify');notify.add_argument('--text',default='New notification');notify.add_argument('--show-content',action='store_true')
    args=parser.parse_args(argv)
    try:
        payload=validate_feed(dict(name=args.name,value=args.value,unit=args.unit,ttl_s=args.ttl)) if args.command=='feed' else notification_payload(args.text,not args.show_content,'integration')
        if args.dry_run:print(json.dumps(payload));return 0
        client=IntegrationClient(args.host,os.environ.get(args.key_env,''),port=args.port)
        result=client.feed(args.slot,args.value,args.name,args.unit,args.ttl) if args.command=='feed' else client.notify(args.text,not args.show_content)
        print(json.dumps(result));return 0
    except (OSError,ValueError) as error:
        parser.exit(1,f'{error}\n')


if __name__=='__main__':raise SystemExit(main())
