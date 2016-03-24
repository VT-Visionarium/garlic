# Important reference documents:
# https://www.barco.com/tde/%280052102360800021%29/TDE6605/00/Barco_UserGuide_TDE6605_00__GP9-platform-LAN-RS232-communication-protocol.pdf
# http://www.barco.com/en/Products/Projectors/Simulation-projectors/Compact-120-Hz-single-chip-DLP-projector.aspx?#!specs
# http://www.barco.com/tde/%280211922361341261%29/601-0307-00/00/Barco_UserGuide_601-0307-00_00__F50-Barco-User-Manual.pdf
#

require 'socket'

# VT CAVE projector IP addresses are programmed into the
# BARCO F50 projectors.  It was done using the projector
# remotes one projector at a time.
$projectorIPs = {
    'LT' =>  '192.168.0.11',
    'LB' =>  '192.168.0.12',
    'CT' =>  '192.168.0.13',
    'CB' =>  '192.168.0.14',
    'RT' =>  '192.168.0.15',
    'RB' =>  '192.168.0.16',
    'FT' =>  '192.168.0.17',
    'FB' =>  '192.168.0.18',
}

def sendToName(cmd,name_in)
    name = name_in.upcase
    ip =  $projectorIPs[name]
    dat = ':' + cmd + "\r"
    s = TCPSocket.open(ip, 1025)
    s.send dat, 0
    puts 'sent ":' + cmd + '\\r" to projector ' + name + ' (ip=' + ip + ')'
    reply = s.recv(1024).strip
    puts 'projector ' + name + ' replied with: ' + reply + "\n"
    # TODO: add return error check
    # a = reply.split(' ')
    #and stuff
end

def send(cmd)
    $projectorIPs.each do |name,ip|
        sendToName(cmd, name)
    end
end

