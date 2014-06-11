class LoadError < ScriptError; end

module Kernel
  def load(path)
    raise NotImplementedError.new "'require' method depends on File"  unless Object.const_defined?(:File)
    raise TypeError  unless path.class == String
    
    if File.exist?(path) && File.extname(path).to_s.downcase == ".mrb"
      _load_mrb_file File.open(path).read.to_s, path
    elsif File.exist?(path)
      _load_rb_str File.open(path).read.to_s, path
    else
      raise LoadError.new "File not found -- #{path}"
    end
  end

  def require(path)
    raise NotImplementedError.new "'require' method depends on File"  unless Object.const_defined?(:File)
    raise TypeError  unless path.class == String

    # require method can load .rb, .mrb or without-ext filename only.
    unless ["", ".rb", ".mrb", ".MRB", ".RB"].include? File.extname(path)
      raise LoadError.new "cannot load such file -- #{path}"
    end

    filenames = []
    if File.extname(path).size == 0
      filenames << "#{path}.rb"
      filenames << "#{path}.mrb"
    else
      filenames << path
    end

    dir = nil
    filename = nil
    if '/' == path[0]
      path0 = filenames.find do |fname|
        fname = fname[1..-1]
        File.file?(fname) && File.exist?(fname)
      end
    elsif '.' == path[0] && '/' == path[1]
      path0 = filenames.find do |fname|
        fname = fname[2..-1]
        File.file?(fname) && File.exist?(fname)
      end
    else
      dir = ($LOAD_PATH || []).find do |dir0|
        filename = filenames.find do |fname|
          path0 = fname
          File.file?(path0) && File.exist?(path0)
        end
      end
      path0 = dir && filename ? filename : nil
    end

    if path0 && File.exist?(path0) && File.file?(path0)
      __require__ path0
    else
      raise LoadError.new "cannot load such file -- #{path}"
    end
  end

  def __require__(realpath)
    raise LoadError.new "File not found -- #{realpath}"  unless File.exist? realpath
    $" ||= []
    $__mruby_loading_files__ ||= []

    # already required
    return false  if ($" + $__mruby_loading_files__).include?(realpath)

    $__mruby_loading_files__ << realpath
    load realpath
    $" << realpath
    $__mruby_loading_files__.delete realpath

    true
  end
end


$LOAD_PATH ||= []
$LOAD_PATH << ''

if Object.const_defined?(:ENV)
  $LOAD_PATH.unshift(*ENV['MRBLIB'].split(':')) unless ENV['MRBLIB'].nil?
end

$LOAD_PATH.uniq!

$" ||= []
$__mruby_loading_files__ ||= []
