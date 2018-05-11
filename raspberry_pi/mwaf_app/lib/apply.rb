class Apply
  def self.reboot
    IO.popen("(sleep 5;sudo reboot)&")
  end

  def self.apply_all(save=true)
    self.apply_wifi_settings(save)
  end

  def self.apply_wifi_settings(save=true)
    wifi_essid = Settings.where(:key => "wifi_essid").first
    if wifi_essid
      temp_conf_file = File.new('/tmp/wpa_supplicant.conf.tmp', 'w')

      wifi_password = Settings.where(:key => "wifi_password").first
      wifi_proto = Settings.where(:key => "wifi_protocol").first
      case wifi_proto.value
        when 'wpa2'
          temp_conf_file.puts 'ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev'
          temp_conf_file.puts 'update_config=1'
          temp_conf_file.puts
          temp_conf_file.puts 'network={'
          temp_conf_file.puts '	ssid="' + wifi_essid.value + '"'
          temp_conf_file.puts '	psk="' + wifi_password.value + '"'
#          temp_conf_file.puts '	proto=WPA2'
          temp_conf_file.puts '	key_mgmt=WPA-PSK'
          temp_conf_file.puts '	id_str="AP1"'
          temp_conf_file.puts '}'
        when 'wpa1'
          temp_conf_file.puts 'ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev'
          temp_conf_file.puts 'update_config=1'
          temp_conf_file.puts
          temp_conf_file.puts 'network={'
          temp_conf_file.puts '	ssid="' + wifi_essid.value + '"'
#          temp_conf_file.puts '	proto=WPA RSN'
          temp_conf_file.puts '	key_mgmt=WPA-PSK'
#          temp_conf_file.puts '	pairwise=CCMP PSK'
#          temp_conf_file.puts '	group=CCMP TKIP'
          temp_conf_file.puts '	psk="' + wifi_password.value + '"'
          temp_conf_file.puts '	id_str="AP1"'
          temp_conf_file.puts '}'
        else
          temp_conf_file.puts 'ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev'
          temp_conf_file.puts 'update_config=1'
          temp_conf_file.puts
          temp_conf_file.puts 'network={'
          temp_conf_file.puts '	ssid="' + wifi_essid.value + '"'
          temp_conf_file.puts '	id_str="AP1"'
          temp_conf_file.puts '}'
      end
      if save
        IO.popen("sudo cp /tmp/wpa_supplicant.conf.tmp /etc/wpa_supplicant/wpa_supplicant.conf") do |pipe|
        end
      end
    end
  end

end