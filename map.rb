i=1
puts '<data xmlns="http://schemas.malighting.de/grandma2/xml/MA">'
File.open(ARGV[0]).each_line do |line|
  line.chomp!
  a = line.split(/\t/)
  if a.size > 1 then
    puts "<key index='#{i}' scan='#{a[0]}' assign='#{a[1]}' />"
    i = i + 1
  end
end
puts "</data>"
