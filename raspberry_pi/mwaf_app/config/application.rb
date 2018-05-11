Mwaf.load_dirs

class Mwaf::Configuration
  def self.database
    "gif_cam.db"
  end
end

class GifCamApplication < Mwaf::Application
  def setup_routes
    get "/settings/wifi", {:controller => "settings", :action => "wifi"}
    get "/settings/save_wifi", {:controller => "settings", :action => "save_wifi"}
    get "/settings/videos", {:controller => "settings", :action => "videos"}
    get "/settings/delete_video", {:controller => "settings", :action => "delete_video"}
    get "/settings/tumblr", {:controller => "settings", :action => "tumblr"}
    post "/settings/save_tumblr", {:controller => "settings", :action => "save_tumblr"}
    get "/settings/send_tumblr", {:controller => "settings", :action => "send_tumblr"}
    get "/settings/reboot", {:controller => "settings", :action => "reboot"}
    get "/settings", {:controller => "settings", :action => "index"}
    get "/", {:controller => "settings", :action => "index"}
  end

  def setup_schema
    table "settings", {:id=>"integer primary key", :key => "varchar(255)", :value => "text"}
  end

end
