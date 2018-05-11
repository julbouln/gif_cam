require 'date'
require 'tumblr_client'
require 'yaml'
require 'pp'

path = "/home/pi/.tumblr"
blog = ARGV[0]
video = ARGV[1]
date = DateTime.strptime(File.basename(video).gsub(/[^\d]+(\d{14}).*/,'\1'),"%Y%m%d%H%M%S")

if File.exist?(path)
  # Load configuration from data
  configuration = YAML.load_file path
  Tumblr.configure do |config|
    Tumblr::Config::VALID_OPTIONS_KEYS.each do |key|
      config.send(:"#{key}=", configuration[key.to_s])
    end
  end
client = Tumblr::Client.new
#pp client.posts(blog)
client.video(blog, {
	:caption => date.strftime("%Y-%m-%d %H:%M:%S"),
	:data => video,
})
end

