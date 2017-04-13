data <- read.csv("manycore.csv")
library(ggplot2)

## ggplot(data, aes(x=threads,y=fpus,shape=precision,color=precision,label=name)) + geom_point(size=4) + geom_text(hjust=0, vjust=0, show_guide=FALSE) + xlab("max. concurrent control flows") + ylab("scalar floating point units") + xlim(0,1250) + ylim(0,3100)
## ggsave("manycore.pdf",width=7,height=3)

data <- subset(data, precision=="DP")
p <- ggplot(data, aes(x=threads,y=fpus,label=name)) + geom_point(size=4, alpha=0.6) + geom_text(hjust=0, vjust=0, show_guide=FALSE) + xlab("max. concurrent control flows") + ylab("double prec. FPUs") + scale_x_log10(breaks=unique(data$threads), limits=c(16,2300)) + scale_y_log10(breaks=unique(data$fpus), limits=c(16,1400))
ggsave("manycore.pdf", p,width=7,height=3)
