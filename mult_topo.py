#!/usr/bin/python

from mininet.net import Mininet, MininetWithControlNet
from mininet.node import Controller, OVSKernelSwitch, RemoteController, OVSSwitch
from mininet.cli import CLI
from mininet.log import setLogLevel, info

net = Mininet(switch=OVSSwitch)






def emptyNet():
    
    net = Mininet(controller=RemoteController, switch=OVSKernelSwitch)

    h1 = net.addHost( 'h1', ip='10.0.0.1' )
    h2 = net.addHost( 'h2', ip='10.0.0.2' )
    h3 = net.addHost( 'h3', ip='10.0.0.3' )
    h4 = net.addHost( 'h4', ip='10.0.0.4' )
    h5 = net.addHost( 'h5', ip='10.0.0.5' )

    s1 = net.addSwitch( 's1')
    s2 = net.addSwitch( 's2')
    s3 = net.addSwitch( 's3')
    s4 = net.addSwitch( 's4')
    s1.linkTo( h1 )
    s1.linkTo( h2 )
    s1.linkTo( s2 )
    s2.linkTo( h3 )
    s2.linkTo( h4 )
    s2.linkTo( s3 )
    s3.linkTo( h5 )
    s4.linkTo( s1 )

    net.build()

    net.start()

    c1 = net.addController('c1', controller=RemoteController, ip="127.0.0.1", port=6653)
    c2 = net.addController('c2', controller=RemoteController, ip="127.0.0.1", port=6657)
    c3 = net.addController('c3', controller=RemoteController, ip="127.0.0.1", port=6658)

    s1.start([c1])
    s2.start([c2])
    s3.start([c2])
    s4.start([c3])

    CLI( net )
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    emptyNet()