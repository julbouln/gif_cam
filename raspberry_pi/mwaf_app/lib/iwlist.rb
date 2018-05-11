#!/usr/bin/ruby
# coding: utf-8

# author : Marc Quinton, march 2013, licence : http://fr.wikipedia.org/wiki/WTFPL

=begin

  Cell 52 - Address: 00:24:D4:51:53:20
			ESSID:"Freebox-A7D027"
			Mode:Master
			Frequency:2.427 GHz (Channel 4)
			Quality=9/70  Signal level=-86 dBm  Noise level=-95 dBm
			Encryption key:on
			Bit Rates:1 Mb/s; 2 Mb/s; 5.5 Mb/s; 11 Mb/s; 6 Mb/s
					  9 Mb/s; 12 Mb/s; 18 Mb/s; 24 Mb/s; 36 Mb/s
					  48 Mb/s; 54 Mb/s
			Extra:bcn_int=96

WPA1
		IE: WPA Version 1
			Group Cipher : CCMP
			Pairwise Ciphers (1) : CCMP
			Authentication Suites (1) : PSK
		Extra:wme_ie=dd180050f2020101000403a4000027a4000042435e0062322f00

WPA2
		IE: IEEE 802.11i/WPA2 Version 1
		Group Cipher : CCMP
		Pairwise Ciphers (1) : CCMP
		Authentication Suites (1) : PSK    

    
=end

class AP
  attr_accessor :cell, :address, :essid, :protocol,
                :mode, :frequency, :channel, :encrypted, :quality,
                :signal_level, :noise_level, :ie, :last_beacon

  def signal_f
    @quality
  end

  def signal
    "#{@quality.round}%"
  end

  def initialize lines
    # @lines = lines.split("\n")
    lines.split("\n").each do |s|
      if m = s.match(/ESSID:"(.*)"/)
        @essid = m[1]
      elsif m = s.match(/Mode:(.*)/)
        @mode = m[1]
      elsif m = s.match(/Frequency:(.*?) \(Channel (\d+)\)/)
        @frequency = m[1]
        @channel = m[2].to_i
      elsif m = s.match(/Address: (.*)/)
        @address = m[1]
      elsif m = s.match(/Encryption key:(.*)/)
        @encrypted = m[1]
        if @encrypted == 'on'
          @protocol = 'wep'
        else
          @protocol = 'none'
        end
      elsif m = s.match(/WPA Version (.*)/)
        @protocol = 'wpa1' if @protocol != "wpa2"
      elsif m = s.match(/WPA2/)
        @protocol = 'wpa2'
        # Quality=9/70  Signal level=-86 dBm  Noise level=-95 dBm
      elsif m = s.match(/Quality=(.*?) .*Signal level=-*(\d+) dBm(\s+Noise level=(.*) dBm)?/)
        @quality = self.quality_to_percent(m[1])
      end

    end
  end

  def quality_to_percent(q)
    n = q.split("/").map(&:to_i)
    return (n[0] * 100) / n[1]
  end

end

class IWList

  def initialize
  end

  def parse content
    aps = []
    content.split(/Cell /m).each do |txt|
      ap = AP.new(txt)
      if ap.quality
        puts "AP: #{ap.inspect}"
        aps << ap
      end

    end

    return aps
  end

  def self.scan(interface)
    parser = self.new
    IO.popen("sudo iwlist #{interface} scan") do |pipe|
      parser.parse pipe.read
    end
  end
end
