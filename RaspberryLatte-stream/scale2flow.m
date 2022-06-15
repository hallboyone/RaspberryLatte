function flow = scale2flow(t,s)
s(1) = 0;
window = 50;
flow = smoothdata(diff(s)./diff(t), 'movmean', window);
flow(end+1) = flow(end);
end