
import time
import sys
import os
import socket
from StockSmart import *
from Gtalk_test import *
from test.test_coercion import format_float

code_list = ['sh600036', 'sh601328']

#windows: GBK
#ubuntu: ascii
print 'System Default Encoding:',sys.getdefaultencoding()

#add this to fix crash when Chinsese input under Ubuntu
reload(sys) 
sys.setdefaultencoding('utf8')


def autoreload():
    mod_names = ['Gtalk_test', 'StockSmart']
    for mod_name in mod_names:
        try:
            module = sys.modules[mod_name]
        except:
            continue
        mtime = os.path.getmtime(module.__file__)
        try:
            if mtime > module.loadtime:
                reload(module)
        except:
            pass
        module.loadtime = mtime


def test_StockSmart():
    test_google()
    test_sina()
    get_stockindex('s_sh000001')
    get_all_price(code_list)
    get_K_char('600036', '1m')

def check_market_open():
    checkopen = False
    text = time.strftime("%H:%M", time.localtime())
    if text > '09:20' and text <= '11:30':
        checkopen = True
    elif text >= '13:00' and text <= '15:00':
        checkopen = True
    print text, checkopen,
    return checkopen

def stock_daemon():
    '''Get the price of stock list, then send it out through Gtalk.
    
    No arguments. '''
    price_old = 0.0
    inited = False
    market_open = False
    
    Gtalk_enable_send(True)
    text = ''
    try:
        while True:
            index = 0
            diff = False
            
            if market_open != check_market_open():
                price_old = 0.
                market_open = check_market_open()
            text = ''
            #get time
            text += time.strftime("%Y-%m-%d %a %H:%M:%S", time.localtime()) + ' '
            if not market_open:
                text += 'Close'
            text += '\n'
            #get price
            for code in code_list:
                name, price_current, price_diff, change_percent = get_price(code)
                if price_current == 0.:
                    print 'get price failed!'
                    break
                if index == 0:
                    if price_old == 0.:
                        price_old = price_current
                        Gtalk_send("stock_daemon v1.0 Online." + socket.gethostname())
                        diff = True
                    if price_current != price_old:
                        diff_ppk = abs((price_current - price_old)*1000/price_old)
                        print 'diff_ppk is', diff_ppk,
                        diff = diff_ppk > 2
                    if diff:
                        price_old = price_current
                text += '%s: %s, %s, %s%%' %(name,price_current, price_diff, change_percent)
                text += '\n'
                index += 1    
            if diff:# or True:
                Gtalk_send(text)
            else:
                print 'same ',
            time.sleep(15)
            if not Gtalk_isRunning():
                Gtalk_send("Gtalk sleep, wakeup it!")
    except KeyboardInterrupt:
        print 'Exception!'
    finally:
        Gtalk_exit()
        time.sleep(1)
     
def get_percent(value, base):
    percent = (value - base) *100 / base
    return '%.4f' % percent 
    
        
def get_price_map():
    k_list = get_K_array('600036', '6m')        
    print k_list
    print len(k_list)
    print len(k_list[1])
    
    idx_date = 1;
    idx_close = 3;
    idx_high = 5;
    idx_low = 7;
    idx_open = 9;
    idx_volume = 11;
    for i in range(1, len(k_list)):
        string = k_list[i][idx_date] + ', '
        string += get_percent( k_list[i][idx_open], k_list[i-1][idx_close]) + ', '
        string += get_percent(k_list[i][idx_high],k_list[i][idx_open]) + ', '
        string += get_percent(k_list[i][idx_low],k_list[i][idx_open])
        print string
        
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''        
if  __name__ == '__main__':
    if len(sys.argv)<2:
        print 'No action specified.'
        print '--1: test_StockSmart'
        print '--2: stock_daemon'
        print '--3: get_price_map'
        sys.exit()
            
    if sys.argv[1].startswith('--'):
        option=sys.argv[1][2:]            
        # fetch sys.argv[1] but without the first two characters
        if option=='1':
            test_StockSmart()
        elif option=='2':
            stock_daemon()    
        elif option=='3':
            get_price_map()
