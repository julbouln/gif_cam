
class SettingsController < Mwaf::Controller
  def index
    alive = IO.popen("gifcam-control alive");
		@wifi_working = false
		ping = IO.popen("ping -c 1 8.8.8.8 2>&1 | grep '0% packet loss'")
		@wifi_working = (ping.read.length > 0)
	end

	def reboot
		Apply.reboot
	end

  def videos
    alive = IO.popen("gifcam-control alive");
    vids_files=Dir.entries("vids").select{|v| v=~/\.mp4$/}.sort.reverse
    @vids = []
    vids_files.each do |f|
      year = 
      @vids << {:filename=>f, :date=>f.scan(/.*_(\d{4})(\d{2})(\d{2})(\d{2})(\d{2})(\d{2}).*/).first}
    end
  end

  def delete_video
    alive = IO.popen("gifcam-control alive");
#    rm = IO.popen("rm -f vids/#{params[:video_filename]}")
    File.delete("vids/#{params[:video_filename]}")

    redirect_to "/settings/videos"
  end

  def wifi
    alive = IO.popen("gifcam-control alive");
    @aps = IWList.scan("wlan0")
    @aps.sort!{|a,b| b.signal_f <=> a.signal_f} # best first

  	@essid = Settings.where(:key=>"wifi_essid").first
  	@essid = @essid ? @essid.value : nil
  end

  def save_wifi
    alive = IO.popen("gifcam-control alive");
#  	puts params
  	essid, protocol = params[:essid].split("--")
  	wifi_essid = Settings.where(:key=>"wifi_essid").first_or_create
  	wifi_essid.value = essid
  	wifi_essid.save

  	wifi_password = Settings.where(:key=>"wifi_password").first_or_create
  	wifi_password.value = params[:password]
  	wifi_password.save

  	wifi_proto = Settings.where(:key=>"wifi_protocol").first_or_create
  	wifi_proto.value = protocol
  	wifi_proto.save

  	redirect_to "/settings/wifi"
  end

  def tumblr
    alive = IO.popen("gifcam-control alive");

    tumblr_blog = Settings.where(:key=>"tumblr_blog").first
    if tumblr_blog
      @blog = tumblr_blog.value
    end

    @api = ""
    if File.exists?("/home/pi/.tumblr")
      @api = File.read("/home/pi/.tumblr")
    end
  end

  def save_tumblr
    alive = IO.popen("gifcam-control alive");

    tumblr_blog = Settings.where(:key=>"tumblr_blog").first_or_create
    if tumblr_blog
      tumblr_blog.value = params[:blog]
      tumblr_blog.save
    end

    # need https://github.com/Asmod4n/mruby-uri-parser
    # show https://github.com/h2o/h2o/issues/993 to update h2o
    api = URI.decode(params[:api]).gsub("+"," ")
    File.open("/home/pi/.tumblr", 'w') { |file| file.write(api) }

    redirect_to "/settings/tumblr"
  end

  def send_tumblr
    alive = IO.popen("gifcam-control alive");
    blog = Settings.where(:key=>"tumblr_blog").first
    video = "/media/videos/#{params[:video_filename]}"

    vid_send_cmd = "ruby /home/pi/tumblr_client.rb #{blog.value} #{video} &"
    vid_send = IO.popen(vid_send_cmd);

    redirect_to "/settings/videos"
  end

end
